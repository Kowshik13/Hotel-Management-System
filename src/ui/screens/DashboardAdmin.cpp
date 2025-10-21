#include "DashboardAdmin.h"
#include "../core/ConsoleIO.h"
#include "../../models/Booking.h"
#include "../../models/BookingStatus.h"
#include "../../models/Hotel.h"
#include "../../models/RestaurantOrderLine.h"
#include "../../models/Room.h"
#include "../../models/RoomStayItem.h"
#include "../../models/User.h"
#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

namespace {

using hms::AppContext;
using hms::Booking;
using hms::BookingStatus;
using hms::Hotel;
using hms::RestaurantOrderLine;
using hms::Room;
using hms::RoomStayItem;
using hms::ui::banner;
using hms::ui::pause;
using hms::ui::readLine;

std::string trimCopy(std::string s) {
    const auto notSpace = [](int ch) { return !std::isspace(ch); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
    s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
    return s;
}

std::optional<int> parseInt(const std::string& text) {
    try {
        size_t idx = 0;
        const int value = std::stoi(text, &idx, 10);
        if (idx == text.size()) return value;
    }
    catch (...) {
    }
    return std::nullopt;
}

std::int64_t nowSeconds() {
    return std::chrono::duration_cast<std::chrono::seconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

std::optional<std::int64_t> parseMoney(const std::string& text) {
    auto s = trimCopy(text);
    if (s.empty()) return std::nullopt;

    if (s.starts_with("₹")) {
        s = trimCopy(s.substr(std::string("₹").size()));
    }
    else if (!s.empty() && s[0] == '$') {
        s.erase(s.begin());
        s = trimCopy(s);
    }

    bool negative = false;
    if (!s.empty() && (s[0] == '-' || s[0] == '+')) {
        negative = (s[0] == '-');
        s.erase(s.begin());
        s = trimCopy(s);
    }

    auto cleanDigits = [](std::string input) {
        input.erase(std::remove_if(input.begin(), input.end(), [](char ch) {
            return ch == ',' || std::isspace(static_cast<unsigned char>(ch));
        }), input.end());
        return input;
    };

    std::string whole;
    std::string fractional;
    if (const auto pos = s.find('.'); pos != std::string::npos) {
        whole = s.substr(0, pos);
        fractional = s.substr(pos + 1);
    }
    else {
        whole = s;
    }

    whole = cleanDigits(whole);
    fractional = cleanDigits(fractional);

    if (whole.empty() && fractional.empty()) return std::nullopt;
    if (whole.empty()) whole = "0";

    try {
        const std::int64_t rupees = std::stoll(whole);
        int paise = 0;
        if (!fractional.empty()) {
            if (fractional.size() > 2) fractional = fractional.substr(0, 2);
            while (fractional.size() < 2) fractional.push_back('0');
            for (char ch : fractional) {
                if (!std::isdigit(static_cast<unsigned char>(ch))) return std::nullopt;
            }
            paise = std::stoi(fractional);
        }

        std::int64_t total = rupees * 100 + paise;
        if (negative) total = -total;
        return total;
    }
    catch (...) {
        return std::nullopt;
    }
}

std::string formatMoney(std::int64_t cents) {
    const bool negative = cents < 0;
    if (negative) cents = std::llabs(cents);

    std::ostringstream oss;
    if (negative) oss << "-";
    oss << "₹" << (cents / 100) << '.'
        << std::setw(2) << std::setfill('0') << (cents % 100);
    return oss.str();
}

std::string formatTimestamp(std::int64_t secs) {
    if (secs <= 0) return "n/a";
    std::time_t t = static_cast<std::time_t>(secs);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M");
    return oss.str();
}

std::string bookingStatusToString(BookingStatus status) {
    switch (status) {
        case BookingStatus::ACTIVE:       return "ACTIVE";
        case BookingStatus::CHECKED_OUT:  return "CHECKED_OUT";
        case BookingStatus::CANCELLED:    return "CANCELLED";
    }
    return "ACTIVE";
}

std::string roleToString(hms::Role role) {
    switch (role) {
        case hms::Role::ADMIN: return "ADMIN";
        case hms::Role::HOTEL_MANAGER: return "HOTEL_MANAGER";
        case hms::Role::GUEST: default: return "GUEST";
    }
}

std::string managerLabel(AppContext& ctx, const std::string& userId) {
    if (userId.empty()) return "-";
    if (auto user = ctx.svc.users->getById(userId)) {
        std::string name = user->firstName + " " + user->lastName;
        name = trimCopy(name);
        if (name.empty()) name = user->login;
        else if (!user->login.empty()) name += " (" + user->login + ")";
        return name;
    }
    return userId;
}

std::optional<Hotel> hotelManagedBy(AppContext& ctx,
                                    const std::string& userId,
                                    const std::string& excludeId) {
    for (const auto& hotel : ctx.svc.hotels->list()) {
        if (hotel.id == excludeId) continue;
        if (hotel.managerUserId == userId) return hotel;
    }
    return std::nullopt;
}

bool assignManagerByLogin(AppContext& ctx, Hotel& hotel, const std::string& login) {
    auto userOpt = ctx.svc.users->getByLogin(login);
    if (!userOpt) {
        std::cout << "No user found with login '" << login << "'.\n";
        return false;
    }

    auto user = *userOpt;
    if (user.role != hms::Role::HOTEL_MANAGER) {
        std::cout << "User is currently " << roleToString(user.role) << ".\n";
        const auto answer = readLine("Convert to HOTEL_MANAGER? (y/N): ", true);
        if (answer != "y" && answer != "Y" && answer != "yes" && answer != "YES") {
            std::cout << "Conversion cancelled.\n";
            return false;
        }
        user.role = hms::Role::HOTEL_MANAGER;
        if (!ctx.svc.users->upsert(user) || !ctx.svc.users->saveAll()) {
            std::cout << "Failed to update user role.\n";
            pause();
            return false;
        }
        std::cout << "User promoted to HOTEL_MANAGER.\n";
    }

    if (auto conflict = hotelManagedBy(ctx, user.userId, hotel.id)) {
        std::cout << "Warning: this manager currently oversees "
                  << conflict->name << " (" << conflict->id << ").\n";
        const auto answer = readLine("Reassign them to the selected hotel? (y/N): ", true);
        if (answer != "y" && answer != "Y" && answer != "yes" && answer != "YES") {
            std::cout << "Assignment cancelled.\n";
            return false;
        }
        auto updated = *conflict;
        updated.managerUserId.clear();
        ctx.svc.hotels->upsert(updated);
    }

    hotel.managerUserId = user.userId;
    return true;
}

std::string nextHotelId(const hms::HotelRepository& repo) {
    int maxValue = 0;
    for (const auto& hotel : repo.list()) {
        const auto pos = hotel.id.find_last_of('-');
        if (pos == std::string::npos) continue;
        const auto tail = hotel.id.substr(pos + 1);
        if (const auto number = parseInt(tail)) {
            maxValue = std::max(maxValue, *number);
        }
    }

    std::ostringstream oss;
    oss << "HTL-" << std::setw(4) << std::setfill('0') << (maxValue + 1);
    return oss.str();
}

std::string join(const std::vector<std::string>& values, const std::string& delim) {
    std::ostringstream oss;
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i != 0) oss << delim;
        oss << values[i];
    }
    return oss.str();
}

std::vector<std::string> splitAmenities(const std::string& text) {
    std::vector<std::string> out;
    std::stringstream ss(text);
    std::string token;
    while (std::getline(ss, token, ',')) {
        token = trimCopy(token);
        if (!token.empty()) out.push_back(token);
    }
    return out;
}

std::string hotelDisplayName(AppContext& ctx, const std::string& hotelId) {
    if (auto hotel = ctx.svc.hotels->get(hotelId)) {
        return hotel->name + " (" + hotel->id + ")";
    }
    return hotelId;
}

bool hasBlockingBookings(AppContext& ctx, const std::string& hotelId) {
    for (const auto& booking : ctx.svc.bookings->listByHotel(hotelId)) {
        if (booking.status != BookingStatus::CANCELLED) return true;
    }
    return false;
}

bool roomHasBlockingBookings(AppContext& ctx, const std::string& hotelId, int roomNumber) {
    for (const auto& booking : ctx.svc.bookings->listByHotel(hotelId)) {
        if (booking.status == BookingStatus::CANCELLED) continue;
        for (const auto& item : booking.items) {
            if (!std::holds_alternative<RoomStayItem>(item)) continue;
            const auto& stay = std::get<RoomStayItem>(item);
            if (stay.hotelId == hotelId && stay.roomNumber == roomNumber) {
                return true;
            }
        }
    }
    return false;
}

void listHotels(AppContext& ctx) {
    banner("Hotels");
    auto hotels = ctx.svc.hotels->list();
    if (hotels.empty()) {
        std::cout << "No hotels found.\n";
        pause();
        return;
    }

    std::sort(hotels.begin(), hotels.end(), [](const Hotel& a, const Hotel& b) {
        return a.id < b.id;
    });

    std::cout << std::left
              << std::setw(6)  << "#"
              << std::setw(12) << "ID"
              << std::setw(26) << "Name"
              << std::setw(7)  << "Stars"
              << std::setw(12) << "Rooms"
              << std::setw(14) << "Base rate"
              << std::setw(26) << "Manager"
              << "Address" << '\n';
    std::cout << std::string(120, '-') << '\n';

    for (std::size_t i = 0; i < hotels.size(); ++i) {
        const auto& hotel = hotels[i];
        const auto roomsForHotel = ctx.svc.rooms->listByHotel(hotel.id);
        const auto activeRooms = ctx.svc.rooms->countActiveByHotel(hotel.id);
        std::ostringstream roomsSummary;
        roomsSummary << activeRooms << "/" << roomsForHotel.size();

        const std::string manager = managerLabel(ctx, hotel.managerUserId);
        const std::string baseRate = hotel.baseRateCents > 0
            ? formatMoney(hotel.baseRateCents)
            : "n/a";

        std::cout << std::left
                  << std::setw(6)  << (i + 1)
                  << std::setw(12) << hotel.id
                  << std::setw(26) << hotel.name.substr(0, 25)
                  << std::setw(7)  << static_cast<int>(hotel.stars)
                  << std::setw(12) << roomsSummary.str()
                  << std::setw(14) << baseRate
                  << std::setw(26) << manager.substr(0, 25)
                  << hotel.address << '\n';
    }

    pause();
}

void createHotel(AppContext& ctx) {
    banner("Create hotel");
    const std::string name = readLine("Hotel name: ");
    const std::string address = readLine("Address: ");

    int stars = 0;
    for (;;) {
        const auto value = readLine("Star rating (1-5): ");
        if (const auto parsed = parseInt(value)) {
            if (*parsed >= 1 && *parsed <= 5) {
                stars = *parsed;
                break;
            }
        }
        std::cout << "Please enter a value between 1 and 5.\n";
    }

    std::int64_t baseRate = 0;
    for (;;) {
        const auto value = readLine("Base nightly rate (€): ");
        if (const auto parsed = parseMoney(value)) {
            baseRate = std::max<std::int64_t>(0, *parsed);
            break;
        }
        std::cout << "Enter an amount such as 8500 or 8499.99.\n";
    }

    Hotel hotel{};
    hotel.id = nextHotelId(*ctx.svc.hotels);
    hotel.name = name;
    hotel.address = address;
    hotel.stars = static_cast<std::uint8_t>(stars);
    hotel.baseRateCents = baseRate;

    for (;;) {
        const auto managerLogin = readLine("Assign manager login (optional): ", true);
        if (managerLogin.empty()) break;
        if (assignManagerByLogin(ctx, hotel, managerLogin)) break;
    }

    ctx.svc.hotels->upsert(hotel);
    ctx.svc.hotels->saveAll();

    std::cout << "Hotel created with ID " << hotel.id << "\n";
    pause();
}

void editHotel(AppContext& ctx) {
    banner("Edit hotel");
    const std::string id = readLine("Hotel ID: ");
    auto hotelOpt = ctx.svc.hotels->get(id);
    if (!hotelOpt) {
        std::cout << "Hotel not found.\n";
        pause();
        return;
    }

    auto hotel = *hotelOpt;
    std::cout << "Editing " << hotel.name << " (" << hotel.id << ")\n";

    const std::string name = readLine("Name [" + hotel.name + "]: ", true);
    if (!name.empty()) hotel.name = name;

    const std::string address = readLine("Address [" + hotel.address + "]: ", true);
    if (!address.empty()) hotel.address = address;

    const std::string starsStr = readLine(
        "Stars (1-5) [" + std::to_string(static_cast<int>(hotel.stars)) + "]: ", true);
    if (!starsStr.empty()) {
        if (const auto parsed = parseInt(starsStr); parsed && *parsed >= 1 && *parsed <= 5) {
            hotel.stars = static_cast<std::uint8_t>(*parsed);
        }
        else {
            std::cout << "Invalid star value. Keeping previous setting.\n";
        }
    }

    for (;;) {
        const auto current = hotel.baseRateCents > 0 ? formatMoney(hotel.baseRateCents) : std::string("n/a");
        const auto input = readLine("Base nightly rate [" + current + "]: ", true);
        if (input.empty()) break;
        if (const auto parsed = parseMoney(input)) {
            hotel.baseRateCents = std::max<std::int64_t>(0, *parsed);
            break;
        }
        std::cout << "Enter an amount such as 850 or 849.99.\n";
    }

    for (;;) {
        std::string currentLogin;
        if (!hotel.managerUserId.empty()) {
            if (auto user = ctx.svc.users->getById(hotel.managerUserId)) {
                currentLogin = user->login;
            }
        }
        std::string prompt = "Manager login";
        if (!currentLogin.empty()) prompt += " [" + currentLogin + "]";
        prompt += " (enter '-' to clear)";

        const auto managerLogin = readLine(prompt + ": ", true);
        if (managerLogin.empty()) break;
        if (managerLogin == "-") {
            hotel.managerUserId.clear();
            break;
        }
        if (assignManagerByLogin(ctx, hotel, managerLogin)) break;
    }

    ctx.svc.hotels->upsert(hotel);
    ctx.svc.hotels->saveAll();

    std::cout << "Hotel updated.\n";
    pause();
}

void removeHotel(AppContext& ctx) {
    banner("Remove hotel");
    const std::string id = readLine("Hotel ID: ");
    auto hotelOpt = ctx.svc.hotels->get(id);
    if (!hotelOpt) {
        std::cout << "Hotel not found.\n";
        pause();
        return;
    }

    const auto& hotel = *hotelOpt;
    const auto roomCount = ctx.svc.rooms->listByHotel(hotel.id).size();
    if (roomCount > 0) {
        std::cout << "This hotel still has " << roomCount << " rooms. Remove rooms first.\n";
        pause();
        return;
    }

    const auto restaurantCount = ctx.svc.restaurants->listByHotel(hotel.id).size();
    if (restaurantCount > 0) {
        std::cout << "This hotel still has " << restaurantCount
                  << " restaurant(s). Remove or reassign them first.\n";
        pause();
        return;
    }

    if (hasBlockingBookings(ctx, hotel.id)) {
        std::cout << "Active or completed bookings exist for this hotel. Deactivate instead of removing.\n";
        pause();
        return;
    }

    const std::string confirmation = readLine("Type DELETE to confirm removal: ");
    if (confirmation != "DELETE") {
        std::cout << "Cancellation confirmed.\n";
        pause();
        return;
    }

    if (!ctx.svc.hotels->remove(hotel.id)) {
        std::cout << "Failed to remove hotel.\n";
        pause();
        return;
    }

    ctx.svc.hotels->saveAll();
    std::cout << "Hotel removed.\n";
    pause();
}

void manageHotels(AppContext& ctx) {
    for (;;) {
        banner("Hotel management");
        std::cout << "1) List hotels\n";
        std::cout << "2) Add hotel\n";
        std::cout << "3) Edit hotel\n";
        std::cout << "4) Remove hotel\n";
        std::cout << "0) Back\n";
        const auto choice = readLine("Select: ");

        if (choice == "0") return;
        if (choice == "1") { listHotels(ctx); continue; }
        if (choice == "2") { createHotel(ctx); continue; }
        if (choice == "3") { editHotel(ctx); continue; }
        if (choice == "4") { removeHotel(ctx); continue; }

        std::cout << "Unknown option.\n";
        pause();
    }
}

void listRoomsForHotel(AppContext& ctx, const Hotel& hotel) {
    banner("Rooms for " + hotel.name);
    auto rooms = ctx.svc.rooms->listByHotel(hotel.id);
    if (rooms.empty()) {
        std::cout << "No rooms configured for this hotel yet.\n";
        pause();
        return;
    }

    std::sort(rooms.begin(), rooms.end(), [](const Room& a, const Room& b) {
        return a.number < b.number;
    });

    std::cout << std::left
              << std::setw(8)  << "Room"
              << std::setw(10) << "Type"
              << std::setw(8)  << "Beds"
              << std::setw(10) << "Size"
              << std::setw(10) << "Active"
              << "Amenities" << '\n';
    std::cout << std::string(80, '-') << '\n';

    for (const auto& room : rooms) {
        std::cout << std::left
                  << std::setw(8)  << room.number
                  << std::setw(10) << room.typeId
                  << std::setw(8)  << room.beds
                  << std::setw(10) << room.sizeSqm
                  << std::setw(10) << (room.active ? "Yes" : "No")
                  << join(room.amenities, ", ") << '\n';
        if (!room.notes.empty()) {
            std::cout << "    Notes: " << room.notes << '\n';
        }
    }

    pause();
}

void addRoom(AppContext& ctx, const Hotel& hotel) {
    banner("Add room to " + hotel.name);
    int number = 0;
    for (;;) {
        const auto value = readLine("Room number: ");
        if (const auto parsed = parseInt(value); parsed && *parsed > 0) {
            number = *parsed;
            if (ctx.svc.rooms->get(hotel.id, number)) {
                std::cout << "Room already exists. Choose another number.\n";
                continue;
            }
            break;
        }
        std::cout << "Enter a positive integer.\n";
    }

    const std::string typeId = readLine("Type (STANDARD/DELUXE/SUITE/...): ");

    int beds = 1;
    for (;;) {
        const auto value = readLine("Beds: ");
        if (const auto parsed = parseInt(value); parsed && *parsed > 0) {
            beds = *parsed;
            break;
        }
        std::cout << "Enter a positive integer.\n";
    }

    int sizeSqm = 0;
    for (;;) {
        const auto value = readLine("Size in sqm: ");
        if (const auto parsed = parseInt(value); parsed && *parsed > 0) {
            sizeSqm = *parsed;
            break;
        }
        std::cout << "Enter a positive integer.\n";
    }

    const std::string amenitiesStr = readLine("Amenities (comma separated): ", true);
    const auto amenities = splitAmenities(amenitiesStr);
    const std::string notes = readLine("Notes (optional): ", true);

    Room room{};
    room.hotelId = hotel.id;
    room.number = number;
    room.typeId = typeId;
    room.beds = beds;
    room.sizeSqm = sizeSqm;
    room.amenities = amenities;
    room.notes = notes;
    room.active = true;
    room.id = hotel.id + "-" + std::to_string(number);

    ctx.svc.rooms->upsert(room);
    ctx.svc.rooms->saveAll();

    std::cout << "Room " << number << " created.\n";
    pause();
}

void editRoom(AppContext& ctx, const Hotel& hotel) {
    banner("Edit room - " + hotel.name);
    const auto roomNumberStr = readLine("Room number: ");
    const auto parsed = parseInt(roomNumberStr);
    if (!parsed) {
        std::cout << "Invalid room number.\n";
        pause();
        return;
    }

    auto roomOpt = ctx.svc.rooms->get(hotel.id, *parsed);
    if (!roomOpt) {
        std::cout << "Room not found.\n";
        pause();
        return;
    }

    auto room = *roomOpt;
    std::cout << "Updating room " << room.number << "\n";

    const std::string type = readLine("Type [" + room.typeId + "]: ", true);
    if (!type.empty()) room.typeId = type;

    const std::string bedsStr = readLine("Beds [" + std::to_string(room.beds) + "]: ", true);
    if (!bedsStr.empty()) {
        if (const auto bedsParsed = parseInt(bedsStr); bedsParsed && *bedsParsed > 0) {
            room.beds = *bedsParsed;
        }
        else {
            std::cout << "Invalid beds value. Keeping previous setting.\n";
        }
    }

    const std::string sizeStr = readLine("Size sqm [" + std::to_string(room.sizeSqm) + "]: ", true);
    if (!sizeStr.empty()) {
        if (const auto sizeParsed = parseInt(sizeStr); sizeParsed && *sizeParsed > 0) {
            room.sizeSqm = *sizeParsed;
        }
        else {
            std::cout << "Invalid size. Keeping previous setting.\n";
        }
    }

    const std::string amenitiesStr = readLine(
        "Amenities (comma list) [" + join(room.amenities, ", ") + "]: ", true);
    if (!amenitiesStr.empty()) room.amenities = splitAmenities(amenitiesStr);

    const std::string notes = readLine("Notes [" + room.notes + "]: ", true);
    if (!notes.empty()) room.notes = notes;

    ctx.svc.rooms->upsert(room);
    ctx.svc.rooms->saveAll();

    std::cout << "Room updated.\n";
    pause();
}

void toggleRoomAvailability(AppContext& ctx, const Hotel& hotel) {
    banner("Toggle room availability");
    const auto roomNumberStr = readLine("Room number: ");
    const auto parsed = parseInt(roomNumberStr);
    if (!parsed) {
        std::cout << "Invalid room number.\n";
        pause();
        return;
    }

    auto roomOpt = ctx.svc.rooms->get(hotel.id, *parsed);
    if (!roomOpt) {
        std::cout << "Room not found.\n";
        pause();
        return;
    }

    auto room = *roomOpt;
    room.active = !room.active;
    ctx.svc.rooms->upsert(room);
    ctx.svc.rooms->saveAll();

    std::cout << "Room " << room.number << (room.active ? " activated." : " deactivated.") << "\n";
    pause();
}

void removeRoom(AppContext& ctx, const Hotel& hotel) {
    banner("Remove room");
    const auto roomNumberStr = readLine("Room number: ");
    const auto parsed = parseInt(roomNumberStr);
    if (!parsed) {
        std::cout << "Invalid room number.\n";
        pause();
        return;
    }

    if (roomHasBlockingBookings(ctx, hotel.id, *parsed)) {
        std::cout << "This room has bookings attached. Set to inactive instead of deleting.\n";
        pause();
        return;
    }

    if (!ctx.svc.rooms->remove(hotel.id, *parsed)) {
        std::cout << "Room not found.\n";
        pause();
        return;
    }

    ctx.svc.rooms->saveAll();
    std::cout << "Room removed.\n";
    pause();
}

void manageRoomsForHotel(AppContext& ctx, const Hotel& hotel) {
    for (;;) {
        banner("Rooms • " + hotel.name);
        std::cout << "1) List rooms\n";
        std::cout << "2) Add room\n";
        std::cout << "3) Edit room\n";
        std::cout << "4) Toggle availability\n";
        std::cout << "5) Remove room\n";
        std::cout << "0) Back\n";
        const auto choice = readLine("Select: ");

        if (choice == "0") return;
        if (choice == "1") { listRoomsForHotel(ctx, hotel); continue; }
        if (choice == "2") { addRoom(ctx, hotel); continue; }
        if (choice == "3") { editRoom(ctx, hotel); continue; }
        if (choice == "4") { toggleRoomAvailability(ctx, hotel); continue; }
        if (choice == "5") { removeRoom(ctx, hotel); continue; }

        std::cout << "Unknown option.\n";
        pause();
    }
}

void manageRooms(AppContext& ctx) {
    for (;;) {
        auto hotels = ctx.svc.hotels->list();
        if (hotels.empty()) {
            banner("Room management");
            std::cout << "Create a hotel before managing rooms.\n";
            pause();
            return;
        }

        std::sort(hotels.begin(), hotels.end(), [](const Hotel& a, const Hotel& b) {
            return a.name < b.name;
        });

        banner("Select hotel");
        for (std::size_t i = 0; i < hotels.size(); ++i) {
            std::cout << (i + 1) << ") " << hotels[i].name << " (" << hotels[i].id << ")\n";
        }
        std::cout << "0) Back\n";

        const auto choice = readLine("Hotel: ");
        if (choice == "0") return;

        if (const auto parsed = parseInt(choice)) {
            if (*parsed >= 1 && static_cast<std::size_t>(*parsed) <= hotels.size()) {
                manageRoomsForHotel(ctx, hotels[static_cast<std::size_t>(*parsed) - 1]);
                continue;
            }
        }

        std::cout << "Invalid selection.\n";
        pause();
    }
}

struct BookingMetrics {
    int rooms{};
    int roomNights{};
    int guests{};
    std::int64_t roomRevenue{};
    std::int64_t diningRevenue{};
};

BookingMetrics summarize(const Booking& booking) {
    BookingMetrics metrics{};
    for (const auto& item : booking.items) {
        if (std::holds_alternative<RoomStayItem>(item)) {
            const auto& stay = std::get<RoomStayItem>(item);
            ++metrics.rooms;
            metrics.roomNights += stay.nights;
            metrics.roomRevenue += stay.nightlyRateLocked * stay.nights;
            metrics.guests += static_cast<int>(stay.occupants.size());
        }
        else if (std::holds_alternative<RestaurantOrderLine>(item)) {
            const auto& order = std::get<RestaurantOrderLine>(item);
            metrics.diningRevenue += order.unitPriceSnapshot * order.qty;
        }
    }
    return metrics;
}

void listBookings(AppContext& ctx, const std::vector<Booking>& sorted) {
    banner("Bookings");
    if (sorted.empty()) {
        std::cout << "No bookings on record.\n";
        pause();
        return;
    }

    std::cout << std::left
              << std::setw(6)  << "#"
              << std::setw(12) << "Booking"
              << std::setw(28) << "Hotel"
              << std::setw(14) << "Status"
              << std::setw(12) << "Nights"
              << std::setw(12) << "Rooms"
              << std::setw(16) << "Revenue"
              << "Created" << '\n';
    std::cout << std::string(110, '-') << '\n';

    for (std::size_t i = 0; i < sorted.size(); ++i) {
        const auto& booking = sorted[i];
        const auto metrics = summarize(booking);
        const std::string hotelName = hotelDisplayName(ctx, booking.hotelId);
        const std::string created = formatTimestamp(booking.createdAt);
        const std::int64_t revenue = metrics.roomRevenue + metrics.diningRevenue;

        std::cout << std::left
                  << std::setw(6)  << (i + 1)
                  << std::setw(12) << booking.bookingId
                  << std::setw(28) << hotelName.substr(0, 27)
                  << std::setw(14) << bookingStatusToString(booking.status)
                  << std::setw(12) << metrics.roomNights
                  << std::setw(12) << metrics.rooms
                  << std::setw(16) << formatMoney(revenue)
                  << created << '\n';
    }

    pause();
}

std::optional<Booking> pickBooking(AppContext& ctx, const std::vector<Booking>& sorted) {
    if (sorted.empty()) {
        std::cout << "No bookings available.\n";
        pause();
        return std::nullopt;
    }

    const auto choice = readLine("Booking number: ");
    if (const auto parsed = parseInt(choice)) {
        if (*parsed >= 1 && static_cast<std::size_t>(*parsed) <= sorted.size()) {
            const auto& selected = sorted[static_cast<std::size_t>(*parsed) - 1];
            if (auto current = ctx.svc.bookings->get(selected.bookingId)) {
                return current;
            }
        }
    }

    std::cout << "Invalid selection.\n";
    pause();
    return std::nullopt;
}

void viewBookingDetails(AppContext& ctx, const Booking& booking) {
    banner("Booking details");
    const auto metrics = summarize(booking);
    std::cout << "Booking ID : " << booking.bookingId << '\n';
    std::cout << "Hotel      : " << hotelDisplayName(ctx, booking.hotelId) << '\n';
    std::cout << "Status     : " << bookingStatusToString(booking.status) << '\n';
    std::cout << "Created    : " << formatTimestamp(booking.createdAt) << '\n';
    std::cout << "Updated    : " << formatTimestamp(booking.updatedAt) << '\n';

    std::string primaryGuest = booking.primaryGuestId;
    if (auto guest = ctx.svc.users->getById(booking.primaryGuestId)) {
        primaryGuest = guest->firstName + " " + guest->lastName + " (" + guest->userId + ")";
    }
    std::cout << "Primary guest: " << primaryGuest << '\n';
    std::cout << "Guests on stay: " << metrics.guests << '\n';
    std::cout << "Room nights   : " << metrics.roomNights << '\n';
    std::cout << "Room revenue  : " << formatMoney(metrics.roomRevenue) << '\n';
    std::cout << "Dining revenue: " << formatMoney(metrics.diningRevenue) << '\n';

    if (booking.items.empty()) {
        std::cout << "No line items captured.\n";
    }
    else {
        std::cout << "\nLine items:\n";
        for (const auto& item : booking.items) {
            if (std::holds_alternative<RoomStayItem>(item)) {
                const auto& stay = std::get<RoomStayItem>(item);
                std::cout << "- Room " << stay.roomNumber
                          << " • " << stay.nights << " night(s) @ "
                          << formatMoney(stay.nightlyRateLocked) << " per night\n";
                if (!stay.occupants.empty()) {
                    std::vector<std::string> names;
                    for (const auto& occ : stay.occupants) {
                        names.push_back(occ.firstName + " " + occ.lastName);
                    }
                    std::cout << "  Guests: " << join(names, ", ") << '\n';
                }
            }
            else {
                const auto& order = std::get<RestaurantOrderLine>(item);
                std::cout << "- Dining • " << order.nameSnapshot
                          << " ×" << order.qty
                          << " • " << formatMoney(order.unitPriceSnapshot * order.qty)
                          << " (" << order.category << ")\n";
            }
        }
    }

    pause();
}

void changeBookingStatus(AppContext& ctx, Booking booking) {
    banner("Update booking status");
    std::cout << "Current status: " << bookingStatusToString(booking.status) << '\n';
    std::cout << "1) Mark ACTIVE\n";
    std::cout << "2) Mark CHECKED_OUT\n";
    std::cout << "3) Mark CANCELLED\n";
    std::cout << "0) Back\n";
    const auto choice = readLine("Select: ");

    BookingStatus newStatus = booking.status;
    if (choice == "0") return;
    if (choice == "1") newStatus = BookingStatus::ACTIVE;
    else if (choice == "2") newStatus = BookingStatus::CHECKED_OUT;
    else if (choice == "3") newStatus = BookingStatus::CANCELLED;
    else {
        std::cout << "Unknown option.\n";
        pause();
        return;
    }

    booking.status = newStatus;
    booking.updatedAt = nowSeconds();

    ctx.svc.bookings->upsert(booking);
    ctx.svc.bookings->saveAll();

    std::cout << "Status updated to " << bookingStatusToString(newStatus) << "\n";
    pause();
}

void purgeBooking(AppContext& ctx, const Booking& booking) {
    banner("Remove booking record");
    if (booking.status != BookingStatus::CANCELLED) {
        std::cout << "Only cancelled bookings can be removed.\n";
        pause();
        return;
    }

    const std::string confirmation = readLine("Type DELETE to confirm: ");
    if (confirmation != "DELETE") {
        std::cout << "Cancelled.\n";
        pause();
        return;
    }

    if (!ctx.svc.bookings->remove(booking.bookingId)) {
        std::cout << "Unable to remove booking.\n";
        pause();
        return;
    }

    ctx.svc.bookings->saveAll();
    std::cout << "Booking removed.\n";
    pause();
}

void manageBookings(AppContext& ctx) {
    for (;;) {
        auto bookings = ctx.svc.bookings->list();
        std::sort(bookings.begin(), bookings.end(), [](const Booking& a, const Booking& b) {
            return a.createdAt > b.createdAt;
        });

        banner("Booking oversight");
        std::cout << "1) List bookings\n";
        std::cout << "2) View booking details\n";
        std::cout << "3) Update booking status\n";
        std::cout << "4) Remove cancelled booking\n";
        std::cout << "0) Back\n";
        const auto choice = readLine("Select: ");

        if (choice == "0") return;
        if (choice == "1") { listBookings(ctx, bookings); continue; }
        if (choice == "2") {
            if (auto booking = pickBooking(ctx, bookings)) viewBookingDetails(ctx, *booking);
            continue;
        }
        if (choice == "3") {
            if (auto booking = pickBooking(ctx, bookings)) changeBookingStatus(ctx, *booking);
            continue;
        }
        if (choice == "4") {
            if (auto booking = pickBooking(ctx, bookings)) purgeBooking(ctx, *booking);
            continue;
        }

        std::cout << "Unknown option.\n";
        pause();
    }
}

void showReports(AppContext& ctx) {
    banner("Operations snapshot");
    const auto hotels = ctx.svc.hotels->list();
    const auto rooms = ctx.svc.rooms->list();
    const auto bookings = ctx.svc.bookings->list();

    int activeRooms = 0;
    for (const auto& room : rooms) if (room.active) ++activeRooms;

    int activeBookings = 0;
    int checkedOutBookings = 0;
    std::int64_t activeRevenue = 0;
    std::int64_t realizedRevenue = 0;
    int activeRoomUsage = 0;

    for (const auto& booking : bookings) {
        const auto metrics = summarize(booking);
        if (booking.status == BookingStatus::ACTIVE) {
            ++activeBookings;
            activeRoomUsage += metrics.rooms;
            activeRevenue += metrics.roomRevenue + metrics.diningRevenue;
        }
        else if (booking.status == BookingStatus::CHECKED_OUT) {
            ++checkedOutBookings;
            realizedRevenue += metrics.roomRevenue + metrics.diningRevenue;
        }
    }

    const int totalRooms = static_cast<int>(rooms.size());
    const double occupancy = (activeRooms > 0)
        ? (static_cast<double>(activeRoomUsage) / static_cast<double>(activeRooms)) * 100.0
        : 0.0;
    std::ostringstream occStream;
    occStream << std::fixed << std::setprecision(1) << occupancy;

    std::cout << "Hotels configured : " << hotels.size() << '\n';
    std::cout << "Rooms total      : " << totalRooms << " (" << activeRooms << " active)\n";
    std::cout << "Active bookings  : " << activeBookings << '\n';
    std::cout << "Checked-out stay : " << checkedOutBookings << '\n';
    std::cout << "Occupancy (live) : " << occStream.str() << "%\n";
    std::cout << "Active pipeline  : " << formatMoney(activeRevenue) << '\n';
    std::cout << "Revenue realized : " << formatMoney(realizedRevenue) << '\n';

    pause();
}

} // namespace

namespace hms::ui {

bool DashboardAdmin(hms::AppContext& ctx) {
    for (;;) {
        banner("Admin dashboard");
        std::cout << "1) Manage hotels\n";
        std::cout << "2) Manage rooms\n";
        std::cout << "3) Booking oversight\n";
        std::cout << "4) Operational reports\n";
        std::cout << "5) Logout\n";
        std::cout << "0) Exit application\n";
        const auto choice = readLine("Select: ");

        if (choice == "5") {
            return true; // logout
        }
        if (choice == "0") {
            ctx.running = false;
            return false; // exit app entirely
        }
        if (choice == "1") { manageHotels(ctx); continue; }
        if (choice == "2") { manageRooms(ctx); continue; }
        if (choice == "3") { manageBookings(ctx); continue; }
        if (choice == "4") { showReports(ctx); continue; }

        std::cout << "Unknown option.\n";
        pause();
    }
}

} // namespace hms::ui
