#include "DashboardGuest.h"
#include "../core/ConsoleIO.h"
#include "../../models/Booking.h"
#include "../../models/BookingStatus.h"
#include "../../models/RoomStayItem.h"
#include "../../models/RestaurantOrderLine.h"
#include "../../security/Security.h"
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
using hms::ui::banner;
using hms::ui::pause;
using hms::ui::readLine;
using hms::ui::readPassword;

struct MenuItem {
    std::string   id;
    std::string   name;
    std::string   category;
    std::int64_t  priceCents{};
};

struct Restaurant {
    std::string        id;
    std::string        name;
    std::string        description;
    std::vector<MenuItem> menu;
};

const std::vector<Restaurant> kRestaurants = {
    {"RST-0001", "Sunrise Cafe", "Fresh breakfasts & light bites", {
         {"bf-omelette",    "Masala Omelette",      "Breakfast",  1800},
         {"bf-pancakes",    "Maple Pancakes",       "Breakfast",  2200},
         {"bf-coldbrew",    "Cold Brew Coffee",     "Drinks",     900},
         {"bf-fruitbowl",   "Seasonal Fruit Bowl",  "Breakfast",  1500},
     }},
    {"RST-0002", "Azure Grill", "Signature grills & sundowner drinks", {
         {"grl-tikka",      "Tandoori Chicken Tikka","Dinner",    3200},
         {"grl-seabass",    "Charred Sea Bass",      "Dinner",    4200},
         {"grl-paneer",     "Smoked Paneer Skewers", "Dinner",    2800},
         {"grl-mojito",     "Classic Mojito",        "Drinks",    1200},
     }}
};

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

std::string formatMoney(std::int64_t cents) {
    const bool negative = cents < 0;
    if (negative) cents = std::llabs(cents);

    std::ostringstream oss;
    if (negative) oss << "-";
    oss << "$" << (cents / 100) << '.'
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

std::string bookingStatusToString(hms::BookingStatus status) {
    switch (status) {
        case hms::BookingStatus::ACTIVE:       return "ACTIVE";
        case hms::BookingStatus::CHECKED_OUT:  return "CHECKED_OUT";
        case hms::BookingStatus::CANCELLED:    return "CANCELLED";
    }
    return "ACTIVE";
}

std::string nextBookingId(const hms::BookingRepository& repo) {
    int maxValue = 0;
    for (const auto& booking : repo.list()) {
        const auto pos = booking.bookingId.find_last_of('-');
        if (pos == std::string::npos) continue;
        const auto tail = booking.bookingId.substr(pos + 1);
        if (const auto number = parseInt(tail)) {
            maxValue = std::max(maxValue, *number);
        }
    }

    std::ostringstream oss;
    oss << "BKG-" << std::setw(6) << std::setfill('0') << (maxValue + 1);
    return oss.str();
}

std::string nextOrderLineId(const hms::Booking& booking) {
    int maxValue = 0;
    for (const auto& item : booking.items) {
        if (!std::holds_alternative<hms::RestaurantOrderLine>(item)) continue;
        const auto& line = std::get<hms::RestaurantOrderLine>(item);
        const auto pos = line.lineId.find_last_of('-');
        if (pos == std::string::npos) continue;
        const auto tail = line.lineId.substr(pos + 1);
        if (const auto number = parseInt(tail)) {
            maxValue = std::max(maxValue, *number);
        }
    }

    std::ostringstream oss;
    oss << "ROL-" << std::setw(4) << std::setfill('0') << (maxValue + 1);
    return oss.str();
}

std::int64_t estimateNightlyRate(const hms::Room& room) {
    std::int64_t base = 9000; // ₹90.00 default
    const std::string type = room.typeId;
    if (type == "DELUXE" || type == "LUXURY") {
        base = 15000;
    }
    else if (type == "SUITE") {
        base = 22000;
    }
    else if (type == "STANDARD") {
        base = 11000;
    }

    base += static_cast<std::int64_t>(std::max(room.beds - 1, 0)) * 2000;
    base += static_cast<std::int64_t>(std::max(room.sizeSqm, 0)) * 70;

    for (const auto& amenity : room.amenities) {
        if (amenity == "Sea View" || amenity == "Balcony") base += 1800;
        if (amenity == "Jacuzzi" || amenity == "Mini Bar") base += 1200;
    }

    return std::max<std::int64_t>(base, 5000);
}

hms::User sanitizeOccupant(const hms::User& user) {
    hms::User copy = user;
    copy.login.clear();
    copy.password.clear();
    copy.passwordHash.clear();
    copy.role = hms::Role::GUEST;
    copy.active = true;
    return copy;
}

hms::User makeExtraOccupant(const std::string& bookingId,
                            int index,
                            const std::string& first,
                            const std::string& last,
                            const hms::User& reference) {
    hms::User guest{};
    guest.userId = bookingId + "-G" + std::to_string(index);
    guest.firstName = first;
    guest.lastName = last;
    guest.address = reference.address;
    guest.phone = reference.phone;
    guest.role = hms::Role::GUEST;
    guest.active = true;
    return guest;
}

std::optional<std::string> promptExtraGuest(const std::string& label) {
    std::cout << label;
    std::string line;
    std::getline(std::cin, line);
    line = trimCopy(line);
    if (line.empty()) return std::nullopt;
    return line;
}

std::vector<hms::Booking> bookingsForGuest(const AppContext& ctx) {
    if (!ctx.currentUser) return {};
    std::vector<hms::Booking> mine;
    for (auto booking : ctx.svc.bookings->list()) {
        if (booking.primaryGuestId == ctx.currentUser->userId) {
            mine.push_back(std::move(booking));
        }
    }
    std::sort(mine.begin(), mine.end(), [](const auto& a, const auto& b) {
        if (a.createdAt == b.createdAt) return a.bookingId > b.bookingId;
        return a.createdAt > b.createdAt;
    });
    return mine;
}

std::string hotelName(const AppContext& ctx, const std::string& hotelId) {
    if (auto h = ctx.svc.hotels->get(hotelId)) {
        return h->name.empty() ? hotelId : h->name;
    }
    return hotelId;
}

std::vector<int> roomNumbersForBooking(const hms::Booking& booking) {
    std::vector<int> numbers;
    for (const auto& item : booking.items) {
        if (std::holds_alternative<hms::RoomStayItem>(item)) {
            const auto& stay = std::get<hms::RoomStayItem>(item);
            numbers.push_back(stay.roomNumber);
        }
    }
    std::sort(numbers.begin(), numbers.end());
    numbers.erase(std::unique(numbers.begin(), numbers.end()), numbers.end());
    return numbers;
}

void showBookingsSummary(const AppContext& ctx, bool verbose) {
    const auto mine = bookingsForGuest(ctx);
    if (mine.empty()) {
        std::cout << "You have no bookings yet.\n";
        return;
    }

    for (const auto& booking : mine) {
        std::cout << "- " << booking.bookingId << " @ "
                  << hotelName(ctx, booking.hotelId)
                  << " (" << bookingStatusToString(booking.status) << ")\n";
        std::cout << "  Created: " << formatTimestamp(booking.createdAt)
                  << ", Updated: " << formatTimestamp(booking.updatedAt) << "\n";

        std::int64_t roomTotal = 0;
        std::int64_t diningTotal = 0;

        for (const auto& item : booking.items) {
            if (std::holds_alternative<hms::RoomStayItem>(item)) {
                const auto& stay = std::get<hms::RoomStayItem>(item);
                const auto total = stay.nightlyRateLocked * stay.nights;
                roomTotal += total;
                std::cout << "    Room " << stay.roomNumber
                          << ": " << stay.nights << " night(s) × "
                          << formatMoney(stay.nightlyRateLocked)
                          << " = " << formatMoney(total) << "\n";
                if (verbose && !stay.occupants.empty()) {
                    std::cout << "      Guests: ";
                    for (std::size_t i = 0; i < stay.occupants.size(); ++i) {
                        const auto& g = stay.occupants[i];
                        std::cout << g.firstName;
                        if (!g.lastName.empty()) std::cout << ' ' << g.lastName;
                        if (i + 1 < stay.occupants.size()) std::cout << ", ";
                    }
                    std::cout << "\n";
                }
            }
            else if (std::holds_alternative<hms::RestaurantOrderLine>(item)) {
                const auto& line = std::get<hms::RestaurantOrderLine>(item);
                const auto total = line.unitPriceSnapshot * line.qty;
                diningTotal += total;
                std::cout << "    Dining: " << line.nameSnapshot
                          << " × " << line.qty
                          << " (" << formatMoney(line.unitPriceSnapshot)
                          << ") -> " << formatMoney(total);
                if (!line.restaurantId.empty()) {
                    std::cout << " @ " << line.restaurantId;
                }
                std::cout << "\n";
            }
        }

        std::cout << "  Room total:   " << formatMoney(roomTotal) << "\n";
        std::cout << "  Dining total: " << formatMoney(diningTotal) << "\n";
        std::cout << "  Grand total:  " << formatMoney(roomTotal + diningTotal) << "\n\n";
    }
}

void handleBookRoom(AppContext& ctx) {
    if (!ctx.currentUser) return;

    auto hotels = ctx.svc.hotels->list();
    if (hotels.empty()) {
        std::cout << "No hotels are available for booking right now.\n";
        pause();
        return;
    }

    std::sort(hotels.begin(), hotels.end(), [](const auto& a, const auto& b) {
        return a.name < b.name;
    });

    banner("Book a room");
    std::cout << "Select a hotel:\n";
    for (std::size_t i = 0; i < hotels.size(); ++i) {
        const auto& h = hotels[i];
        std::cout << "  " << (i + 1) << ") " << h.name
                  << " — " << h.address << "\n";
    }
    std::cout << "  0) Cancel\n";

    const auto hotelChoice = parseInt(readLine("Hotel: "));
    if (!hotelChoice || *hotelChoice < 0 || *hotelChoice > static_cast<int>(hotels.size())) {
        std::cout << "Invalid selection.\n";
        pause();
        return;
    }
    if (*hotelChoice == 0) return;

    const auto selectedHotel = hotels[*hotelChoice - 1];
    auto rooms = ctx.svc.rooms->listByHotel(selectedHotel.id);
    rooms.erase(std::remove_if(rooms.begin(), rooms.end(), [](const hms::Room& r) {
                    return !r.active;
                }),
                rooms.end());

    if (rooms.empty()) {
        std::cout << "No active rooms are available in this hotel.\n";
        pause();
        return;
    }

    std::sort(rooms.begin(), rooms.end(), [](const auto& a, const auto& b) {
        return a.number < b.number;
    });

    std::cout << "\nAvailable rooms at " << selectedHotel.name << ":\n";
    for (std::size_t i = 0; i < rooms.size(); ++i) {
        const auto& room = rooms[i];
        const auto rate = estimateNightlyRate(room);
        std::cout << "  " << (i + 1) << ") Room " << room.number
                  << " • Beds: " << room.beds
                  << " • Size: " << room.sizeSqm << " sqm"
                  << " • Rate: " << formatMoney(rate);
        if (!room.amenities.empty()) {
            std::cout << " • Amenities: ";
            for (std::size_t a = 0; a < room.amenities.size(); ++a) {
                std::cout << room.amenities[a];
                if (a + 1 < room.amenities.size()) std::cout << ", ";
            }
        }
        std::cout << "\n";
    }
    std::cout << "  0) Cancel\n";

    const auto roomChoice = parseInt(readLine("Room: "));
    if (!roomChoice || *roomChoice < 0 || *roomChoice > static_cast<int>(rooms.size())) {
        std::cout << "Invalid selection.\n";
        pause();
        return;
    }
    if (*roomChoice == 0) return;

    const auto selectedRoom = rooms[*roomChoice - 1];
    const auto nights = parseInt(readLine("Number of nights (1-30): "));
    if (!nights || *nights <= 0 || *nights > 30) {
        std::cout << "Please choose between 1 and 30 nights.\n";
        pause();
        return;
    }

    const auto nightlyRate = estimateNightlyRate(selectedRoom);
    const auto totalCost   = nightlyRate * (*nights);

    const auto bookingId = nextBookingId(*ctx.svc.bookings);

    hms::Booking booking{};
    booking.bookingId = bookingId;
    booking.hotelId = selectedHotel.id;
    booking.status = hms::BookingStatus::ACTIVE;
    booking.primaryGuestId = ctx.currentUser->userId;
    booking.createdAt = booking.updatedAt = nowSeconds();

    hms::RoomStayItem stay{};
    stay.hotelId = selectedHotel.id;
    stay.roomNumber = selectedRoom.number;
    stay.nights = *nights;
    stay.nightlyRateLocked = nightlyRate;
    stay.occupants.push_back(sanitizeOccupant(*ctx.currentUser));

    int extraIndex = 1;
    while (true) {
        const auto prompt = std::string("Add another guest full name (or press Enter to continue): ");
        const auto extra = promptExtraGuest(prompt);
        if (!extra) break;

        std::istringstream iss(*extra);
        std::string first, last;
        iss >> first;
        std::getline(iss, last);
        first = trimCopy(first);
        last  = trimCopy(last);
        if (first.empty()) {
            std::cout << "Skipping empty guest entry.\n";
            continue;
        }
        ++extraIndex;
        stay.occupants.push_back(makeExtraOccupant(bookingId, extraIndex, first, last, *ctx.currentUser));
    }

    booking.items.push_back(stay);

    std::cout << "\nSummary:\n";
    std::cout << "  Hotel:  " << selectedHotel.name << "\n";
    std::cout << "  Room:   " << selectedRoom.number << " for " << *nights
              << " night(s)\n";
    std::cout << "  Guests: " << stay.occupants.size() << "\n";
    std::cout << "  Total:  " << formatMoney(totalCost) << "\n";

    auto confirm = readLine("Confirm this booking? (y/N): ", /*allowEmpty=*/true);
    std::transform(confirm.begin(), confirm.end(), confirm.begin(), [](unsigned char ch){ return static_cast<char>(std::tolower(ch)); });
    if (confirm != "y" && confirm != "yes") {
        std::cout << "Booking cancelled.\n";
        pause();
        return;
    }

    if (!ctx.svc.bookings->upsert(booking)) {
        std::cout << "Failed to store booking.\n";
        pause();
        return;
    }
    if (!ctx.svc.bookings->saveAll()) {
        std::cout << "Warning: booking saved in memory but not on disk.\n";
    }

    std::cout << "Booking confirmed! Your reference is " << booking.bookingId << "\n";
    pause();
}

void handleRestaurantReservation(AppContext& ctx) {
    if (!ctx.currentUser) return;

    auto mine = bookingsForGuest(ctx);
    if (mine.empty()) {
        std::cout << "You need an active room booking before placing a restaurant order.\n";
        pause();
        return;
    }

    banner("Reserve restaurant");
    std::cout << "Choose the booking to bill:\n";
    for (std::size_t i = 0; i < mine.size(); ++i) {
        const auto& booking = mine[i];
        std::cout << "  " << (i + 1) << ") " << booking.bookingId
                  << " @ " << hotelName(ctx, booking.hotelId)
                  << " (Rooms: ";
        const auto rooms = roomNumbersForBooking(booking);
        if (rooms.empty()) std::cout << "-";
        else {
            for (std::size_t r = 0; r < rooms.size(); ++r) {
                std::cout << rooms[r];
                if (r + 1 < rooms.size()) std::cout << ", ";
            }
        }
        std::cout << ")\n";
    }
    std::cout << "  0) Cancel\n";

    const auto bookingChoice = parseInt(readLine("Booking: "));
    if (!bookingChoice || *bookingChoice < 0 || *bookingChoice > static_cast<int>(mine.size())) {
        std::cout << "Invalid selection.\n";
        pause();
        return;
    }
    if (*bookingChoice == 0) return;

    const auto bookingId = mine[*bookingChoice - 1].bookingId;
    auto bookingOpt = ctx.svc.bookings->get(bookingId);
    if (!bookingOpt) {
        std::cout << "Booking could not be loaded.\n";
        pause();
        return;
    }

    auto booking = *bookingOpt;
    std::cout << "\nAvailable restaurants:\n";
    for (std::size_t i = 0; i < kRestaurants.size(); ++i) {
        const auto& r = kRestaurants[i];
        std::cout << "  " << (i + 1) << ") " << r.name
                  << " — " << r.description << "\n";
    }
    std::cout << "  0) Cancel\n";

    const auto restaurantChoice = parseInt(readLine("Restaurant: "));
    if (!restaurantChoice || *restaurantChoice < 0 || *restaurantChoice > static_cast<int>(kRestaurants.size())) {
        std::cout << "Invalid selection.\n";
        pause();
        return;
    }
    if (*restaurantChoice == 0) return;

    const auto& restaurant = kRestaurants[*restaurantChoice - 1];

    bool addedSomething = false;
    std::int64_t orderTotal = 0;
    for (;;) {
        std::cout << "\n" << restaurant.name << " menu:\n";
        for (std::size_t i = 0; i < restaurant.menu.size(); ++i) {
            const auto& item = restaurant.menu[i];
            std::cout << "  " << (i + 1) << ") [" << item.category << "] "
                      << item.name << " — " << formatMoney(item.priceCents) << "\n";
        }
        std::cout << "  0) Finish order\n";

        const auto itemChoice = parseInt(readLine("Item: "));
        if (!itemChoice || *itemChoice < 0 || *itemChoice > static_cast<int>(restaurant.menu.size())) {
            std::cout << "Invalid selection.\n";
            continue;
        }
        if (*itemChoice == 0) break;

        const auto& menuItem = restaurant.menu[*itemChoice - 1];
        const auto qty = parseInt(readLine("Quantity (1-10): "));
        if (!qty || *qty <= 0 || *qty > 10) {
            std::cout << "Please choose a quantity between 1 and 10.\n";
            continue;
        }

        auto rooms = roomNumbersForBooking(booking);
        int billedRoom = 0;
        if (!rooms.empty()) {
            std::cout << "Bill to which room?\n";
            for (std::size_t i = 0; i < rooms.size(); ++i) {
                std::cout << "  " << (i + 1) << ") Room " << rooms[i] << "\n";
            }
            std::cout << "  0) No room (walk-in)\n";
            const auto roomChoice = parseInt(readLine("Room: "));
            if (roomChoice && *roomChoice > 0 && *roomChoice <= static_cast<int>(rooms.size())) {
                billedRoom = rooms[*roomChoice - 1];
            }
        }

        hms::RestaurantOrderLine line{};
        line.lineId = nextOrderLineId(booking);
        line.restaurantId = restaurant.id;
        line.category = menuItem.category;
        line.menuItemId = menuItem.id;
        line.nameSnapshot = menuItem.name;
        line.unitPriceSnapshot = menuItem.priceCents;
        line.qty = *qty;
        line.billedRoomNumber = billedRoom;
        line.takenByUsername = restaurant.id + "-desk";
        line.orderedByGuestId = ctx.currentUser->userId;
        line.createdAt = nowSeconds();

        booking.items.push_back(line);
        booking.updatedAt = line.createdAt;
        addedSomething = true;
        orderTotal += line.unitPriceSnapshot * line.qty;

        std::cout << "Added " << menuItem.name << " × " << *qty
                  << " — running total " << formatMoney(orderTotal) << "\n";

        auto again = readLine("Add another item? (y/N): ", /*allowEmpty=*/true);
        std::transform(again.begin(), again.end(), again.begin(), [](unsigned char ch){ return static_cast<char>(std::tolower(ch)); });
        if (again != "y" && again != "yes") break;
    }

    if (!addedSomething) {
        std::cout << "No items added.\n";
        pause();
        return;
    }

    if (!ctx.svc.bookings->upsert(booking)) {
        std::cout << "Failed to update booking with restaurant order.\n";
        pause();
        return;
    }
    if (!ctx.svc.bookings->saveAll()) {
        std::cout << "Warning: order saved in memory but not on disk.\n";
    }

    std::cout << "Reservation saved! Total dining spend: " << formatMoney(orderTotal) << "\n";
    pause();
}

std::string promptWithDefault(const std::string& label, const std::string& current) {
    std::cout << label << " [" << current << "]: ";
    std::string line;
    std::getline(std::cin, line);
    line = trimCopy(line);
    return line.empty() ? current : line;
}

void editProfile(AppContext& ctx) {
    if (!ctx.currentUser) return;

    auto updated = *ctx.currentUser;
    banner("Edit profile");
    updated.firstName = promptWithDefault("First name", updated.firstName);
    updated.lastName  = promptWithDefault("Last name", updated.lastName);
    updated.address   = promptWithDefault("Address", updated.address);
    updated.phone     = promptWithDefault("Phone", updated.phone);

    if (updated.firstName == ctx.currentUser->firstName &&
        updated.lastName  == ctx.currentUser->lastName &&
        updated.address   == ctx.currentUser->address &&
        updated.phone     == ctx.currentUser->phone) {
        std::cout << "No changes detected.\n";
        pause();
        return;
    }

    if (!ctx.svc.users->upsert(updated)) {
        std::cout << "Failed to update profile.\n";
        pause();
        return;
    }
    if (!ctx.svc.users->saveAll()) {
        std::cout << "Warning: profile saved in memory but not on disk.\n";
    }

    ctx.currentUser = updated;
    std::cout << "Profile updated successfully.\n";
    pause();
}

void changePassword(AppContext& ctx) {
    if (!ctx.currentUser) return;

    banner("Change password");
    const auto current = readPassword("Current password: ");
    if (!hms::VerifyPasswordDemo(ctx.currentUser->passwordHash, current)) {
        std::cout << "Incorrect password.\n";
        pause();
        return;
    }

    const auto newPass = readPassword("New password (min 6 chars): ");
    if (newPass.size() < 6) {
        std::cout << "Password must be at least 6 characters.\n";
        pause();
        return;
    }
    const auto confirm = readPassword("Confirm new password: ");
    if (newPass != confirm) {
        std::cout << "Passwords do not match.\n";
        pause();
        return;
    }

    auto updated = *ctx.currentUser;
    updated.passwordHash = hms::HashPasswordDemo(newPass);
    updated.password.clear();

    if (!ctx.svc.users->upsert(updated)) {
        std::cout << "Failed to update password.\n";
        pause();
        return;
    }
    if (!ctx.svc.users->saveAll()) {
        std::cout << "Warning: password saved in memory but not on disk.\n";
    }

    ctx.currentUser = updated;
    std::cout << "Password changed successfully.\n";
    pause();
}

void showProfileHeader(const AppContext& ctx) {
    if (!ctx.currentUser) return;
    const auto& user = *ctx.currentUser;
    std::cout << "Logged in as: " << user.firstName;
    if (!user.lastName.empty()) std::cout << ' ' << user.lastName;
    std::cout << " (" << user.login << ")\n";
    std::cout << "Phone: " << (user.phone.empty() ? "n/a" : user.phone) << "\n";
    std::cout << "Address: " << (user.address.empty() ? "n/a" : user.address) << "\n";
    std::cout << "------------------------------\n";
}

void handleProfile(AppContext& ctx) {
    if (!ctx.currentUser) return;

    for (;;) {
        banner("Profile & bookings");
        showProfileHeader(ctx);
        std::cout << "1) View my bookings\n";
        std::cout << "2) Edit contact details\n";
        std::cout << "3) Change password\n";
        std::cout << "0) Back\n";

        const auto choice = readLine("Select: ");
        if (choice == "1") {
            banner("My bookings");
            showBookingsSummary(ctx, /*verbose=*/true);
            pause();
        }
        else if (choice == "2") {
            editProfile(ctx);
        }
        else if (choice == "3") {
            changePassword(ctx);
        }
        else if (choice == "0") {
            return;
        }
        else {
            std::cout << "Please choose one of the options above.\n";
            pause();
        }
    }
}

} // namespace

namespace hms::ui {

bool DashboardGuest(hms::AppContext& ctx) {
    for (;;) {
        banner("Guest Dashboard");
        std::cout << "1) Book a room\n";
        std::cout << "2) Reserve restaurant\n";
        std::cout << "3) Profile & bookings\n";
        std::cout << "4) Logout\n";
        std::cout << "0) Exit application\n";
        const auto choice = readLine("Select: ");

        if (choice == "1") {
            handleBookRoom(ctx);
            continue;
        }
        if (choice == "2") {
            handleRestaurantReservation(ctx);
            continue;
        }
        if (choice == "3") {
            handleProfile(ctx);
            continue;
        }
        if (choice == "4") {
            return true;
        }
        if (choice == "0") {
            ctx.running = false;
            return false;
        }

        std::cout << "Please choose one of the options above.\n";
        pause();
    }
}

} // namespace hms::ui
