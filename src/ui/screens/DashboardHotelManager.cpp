#include "DashboardHotelManager.h"

#include "../core/ConsoleIO.h"
#include "../../models/Booking.h"
#include "../../models/BookingStatus.h"
#include "../../models/Hotel.h"
#include "../../models/MenuItem.h"
#include "../../models/Restaurant.h"
#include "../../models/RestaurantOrderLine.h"
#include "../../models/Room.h"
#include "../../models/RoomStayItem.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>
#include <ctime>

namespace {

using hms::AppContext;
using hms::Booking;
using hms::BookingStatus;
using hms::Hotel;
using hms::MenuItem;
using hms::Restaurant;
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

std::optional<std::int64_t> parseMoney(const std::string& text) {
    auto s = trimCopy(text);
    if (s.empty()) return std::nullopt;
    if (!s.empty() && s[0] == '$') {
        s.erase(s.begin());
        s = trimCopy(s);
    }
    else if (s.size() >= 3) {
        std::string prefix = s.substr(0, 3);
        std::transform(prefix.begin(), prefix.end(), prefix.begin(),
                       [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
        if (prefix == "eur") {
            s = trimCopy(s.substr(3));
        }
    }
    bool negative = false;
    if (!s.empty() && (s[0] == '-' || s[0] == '+')) {
        negative = (s[0] == '-');
        s.erase(s.begin());
        s = trimCopy(s);
    }
    auto clean = [](std::string v) {
        v.erase(std::remove_if(v.begin(), v.end(), [](char ch) {
            return ch == ',' || std::isspace(static_cast<unsigned char>(ch));
        }), v.end());
        return v;
    };
    std::string whole;
    std::string fractional;
    if (const auto pos = s.find('.'); pos != std::string::npos) {
        whole = s.substr(0, pos);
        fractional = s.substr(pos + 1);
    } else {
        whole = s;
    }
    whole = clean(whole);
    fractional = clean(fractional);
    if (whole.empty() && fractional.empty()) return std::nullopt;
    if (whole.empty()) whole = "0";
    try {
        const std::int64_t euros = std::stoll(whole);
        int centsPart = 0;
        if (!fractional.empty()) {
            if (fractional.size() > 2) fractional = fractional.substr(0, 2);
            while (fractional.size() < 2) fractional.push_back('0');
            for (char ch : fractional) {
                if (!std::isdigit(static_cast<unsigned char>(ch))) return std::nullopt;
            }
            centsPart = std::stoi(fractional);
        }
        std::int64_t total = euros * 100 + centsPart;
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
    oss << "EUR " << (cents / 100) << '.'
        << std::setw(2) << std::setfill('0') << (cents % 100);
    return oss.str();
}

std::string bookingStatusToString(BookingStatus status) {
    switch (status) {
        case BookingStatus::ACTIVE: return "ACTIVE";
        case BookingStatus::CHECKED_OUT: return "CHECKED_OUT";
        case BookingStatus::CANCELLED: return "CANCELLED";
    }
    return "ACTIVE";
}

std::optional<Hotel> managedHotel(AppContext& ctx) {
    if (!ctx.currentUser) return std::nullopt;
    for (const auto& hotel : ctx.svc.hotels->list()) {
        if (hotel.managerUserId == ctx.currentUser->userId) {
            return hotel;
        }
    }
    return std::nullopt;
}

std::string nextRestaurantId(const hms::RestaurantRepository& repo) {
    int maxValue = 0;
    for (const auto& restaurant : repo.list()) {
        const auto pos = restaurant.id.find_last_of('-');
        if (pos == std::string::npos) continue;
        const auto tail = restaurant.id.substr(pos + 1);
        if (const auto parsed = parseInt(tail)) {
            maxValue = std::max(maxValue, *parsed);
        }
    }
    std::ostringstream oss;
    oss << "RST-" << std::setw(4) << std::setfill('0') << (maxValue + 1);
    return oss.str();
}

std::string nextMenuItemId(const Restaurant& restaurant) {
    int maxValue = 0;
    for (const auto& item : restaurant.menu) {
        const auto pos = item.id.find_last_of('-');
        if (pos == std::string::npos) continue;
        const auto tail = item.id.substr(pos + 1);
        if (const auto parsed = parseInt(tail)) {
            maxValue = std::max(maxValue, *parsed);
        }
    }
    std::ostringstream oss;
    oss << restaurant.id << "-M" << std::setw(3) << std::setfill('0') << (maxValue + 1);
    return oss.str();
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

void showSummary(AppContext& ctx, const Hotel& hotel) {
    banner("Hotel snapshot");
    std::cout << "Hotel       : " << hotel.name << " (" << hotel.id << ")\n";
    std::cout << "Address     : " << hotel.address << "\n";
    std::cout << "Stars       : " << static_cast<int>(hotel.stars) << "\n";
    std::cout << "Base rate   : " << (hotel.baseRateCents > 0 ? formatMoney(hotel.baseRateCents) : std::string("n/a")) << "\n";

    const auto rooms = ctx.svc.rooms->listByHotel(hotel.id);
    const auto activeRooms = ctx.svc.rooms->countActiveByHotel(hotel.id);
    std::cout << "Rooms       : " << activeRooms << " active / " << rooms.size() << " total\n";

    const auto restaurants = ctx.svc.restaurants->listByHotel(hotel.id);
    int activeRestaurants = 0;
    for (const auto& r : restaurants) if (r.active) ++activeRestaurants;
    std::cout << "Restaurants : " << activeRestaurants << " active / " << restaurants.size() << " total\n";

    const auto bookings = ctx.svc.bookings->listByHotel(hotel.id);
    int activeBookings = 0;
    std::int64_t roomRevenue = 0;
    std::int64_t diningRevenue = 0;
    for (const auto& booking : bookings) {
        if (booking.status == BookingStatus::ACTIVE) ++activeBookings;
        const auto metrics = summarize(booking);
        roomRevenue += metrics.roomRevenue;
        diningRevenue += metrics.diningRevenue;
    }
    std::cout << "Bookings    : " << bookings.size() << " total / " << activeBookings << " active\n";
    std::cout << "Revenue     : " << formatMoney(roomRevenue + diningRevenue)
              << " (rooms " << formatMoney(roomRevenue)
              << ", dining " << formatMoney(diningRevenue) << ")\n";

    pause();
}

bool roomHasBlockingBookings(AppContext& ctx, const Hotel& hotel, int roomNumber) {
    for (const auto& booking : ctx.svc.bookings->listByHotel(hotel.id)) {
        if (booking.status == BookingStatus::CANCELLED) continue;
        for (const auto& item : booking.items) {
            if (!std::holds_alternative<RoomStayItem>(item)) continue;
            const auto& stay = std::get<RoomStayItem>(item);
            if (stay.hotelId == hotel.id && stay.roomNumber == roomNumber) {
                return true;
            }
        }
    }
    return false;
}

void listRooms(AppContext& ctx, const Hotel& hotel) {
    banner("Rooms at " + hotel.name);
    auto rooms = ctx.svc.rooms->listByHotel(hotel.id);
    if (rooms.empty()) {
        std::cout << "No rooms configured yet.\n";
        pause();
        return;
    }
    std::sort(rooms.begin(), rooms.end(), [](const Room& a, const Room& b) {
        return a.number < b.number;
    });
    std::cout << std::left
              << std::setw(8)  << "Room"
              << std::setw(10) << "Type"
              << std::setw(6)  << "Beds"
              << std::setw(8)  << "Size"
              << std::setw(10) << "Active"
              << "Amenities" << '\n';
    std::cout << std::string(80, '-') << '\n';
    for (const auto& room : rooms) {
        std::cout << std::left
                  << std::setw(8)  << room.number
                  << std::setw(10) << room.typeId
                  << std::setw(6)  << room.beds
                  << std::setw(8)  << room.sizeSqm
                  << std::setw(10) << (room.active ? "Yes" : "No")
                  << [&](){
                        std::ostringstream oss;
                        for (std::size_t i = 0; i < room.amenities.size(); ++i) {
                            oss << room.amenities[i];
                            if (i + 1 < room.amenities.size()) oss << ", ";
                        }
                        return oss.str();
                     }()
                  << '\n';
        if (!room.notes.empty()) {
            std::cout << "        Notes: " << room.notes << '\n';
        }
    }
    pause();
}

bool ensureRoomNumberUnique(const AppContext& ctx, const Hotel& hotel, int number, int previous) {
    if (number == previous) return true;
    if (number <= 0) return false;
    return !ctx.svc.rooms->get(hotel.id, number).has_value();
}

void addRoom(AppContext& ctx, const Hotel& hotel) {
    banner("Add room");
    int number = 0;
    for (;;) {
        const auto input = readLine("Room number: ");
        if (const auto parsed = parseInt(input)) {
            if (*parsed > 0 && ensureRoomNumberUnique(ctx, hotel, *parsed, -1)) {
                number = *parsed;
                break;
            }
        }
        std::cout << "Provide a positive number that is not in use.\n";
    }
    Room room{};
    room.hotelId = hotel.id;
    room.number = number;
    room.id = hotel.id + "-" + std::to_string(number);
    room.typeId = readLine("Room type (e.g., STANDARD/DELUXE): ");
    if (const auto beds = parseInt(readLine("Beds (1-6): "))) {
        room.beds = std::clamp(*beds, 1, 6);
    }
    if (const auto size = parseInt(readLine("Size in sqm: "))) {
        room.sizeSqm = std::max(0, *size);
    }
    const auto amenitiesRaw = readLine("Amenities (comma separated): ", true);
    std::stringstream ss(amenitiesRaw);
    std::string token;
    while (std::getline(ss, token, ',')) {
        token = trimCopy(token);
        if (!token.empty()) room.amenities.push_back(token);
    }
    room.notes = readLine("Notes (optional): ", true);
    room.active = true;
    ctx.svc.rooms->upsert(room);
    ctx.svc.rooms->saveAll();
    std::cout << "Room " << room.number << " added.\n";
    pause();
}

void editRoom(AppContext& ctx, const Hotel& hotel) {
    banner("Edit room");
    const auto input = readLine("Room number: ");
    if (const auto parsed = parseInt(input)) {
        auto roomOpt = ctx.svc.rooms->get(hotel.id, *parsed);
        if (!roomOpt) {
            std::cout << "Room not found.\n";
            pause();
            return;
        }
        auto room = *roomOpt;
        const auto type = readLine("Type [" + room.typeId + "]: ", true);
        if (!type.empty()) room.typeId = type;
        const auto beds = readLine("Beds [" + std::to_string(room.beds) + "]: ", true);
        if (!beds.empty()) {
            if (const auto parsedBeds = parseInt(beds)) {
                room.beds = std::clamp(*parsedBeds, 1, 6);
            }
        }
        const auto size = readLine("Size sqm [" + std::to_string(room.sizeSqm) + "]: ", true);
        if (!size.empty()) {
            if (const auto parsedSize = parseInt(size)) {
                room.sizeSqm = std::max(0, *parsedSize);
            }
        }
        const auto amenities = readLine("Amenities (comma separated) [unchanged]: ", true);
        if (!amenities.empty()) {
            room.amenities.clear();
            std::stringstream ss(amenities);
            std::string token;
            while (std::getline(ss, token, ',')) {
                token = trimCopy(token);
                if (!token.empty()) room.amenities.push_back(token);
            }
        }
        const auto notes = readLine("Notes [" + room.notes + "]: ", true);
        if (!notes.empty()) room.notes = notes;
        ctx.svc.rooms->upsert(room);
        ctx.svc.rooms->saveAll();
        std::cout << "Room updated.\n";
        pause();
        return;
    }
    std::cout << "Invalid room number.\n";
    pause();
}

void toggleRoom(AppContext& ctx, const Hotel& hotel) {
    banner("Toggle room availability");
    const auto input = readLine("Room number: ");
    if (const auto parsed = parseInt(input)) {
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
        std::cout << "Room " << room.number << (room.active ? " activated." : " deactivated.") << '\n';
        pause();
        return;
    }
    std::cout << "Invalid room number.\n";
    pause();
}

void removeRoom(AppContext& ctx, const Hotel& hotel) {
    banner("Remove room");
    const auto input = readLine("Room number: ");
    if (const auto parsed = parseInt(input)) {
        auto roomOpt = ctx.svc.rooms->get(hotel.id, *parsed);
        if (!roomOpt) {
            std::cout << "Room not found.\n";
            pause();
            return;
        }
        if (roomHasBlockingBookings(ctx, hotel, *parsed)) {
            std::cout << "Cannot remove this room because bookings reference it. Mark it inactive instead.\n";
            pause();
            return;
        }

        ctx.svc.rooms->remove(hotel.id, *parsed);
        ctx.svc.rooms->saveAll();
        std::cout << "Room removed.\n";
        pause();
        return;
    }
    std::cout << "Invalid room number.\n";
    pause();
}

void manageRooms(AppContext& ctx, const Hotel& hotel) {
    for (;;) {
        banner("Manage rooms");
        std::cout << "1) List rooms\n";
        std::cout << "2) Add room\n";
        std::cout << "3) Edit room\n";
        std::cout << "4) Toggle availability\n";
        std::cout << "5) Remove room\n";
        std::cout << "0) Back\n";
        const auto choice = readLine("Select: ");
        if (choice == "0") return;
        if (choice == "1") { listRooms(ctx, hotel); continue; }
        if (choice == "2") { addRoom(ctx, hotel); continue; }
        if (choice == "3") { editRoom(ctx, hotel); continue; }
        if (choice == "4") { toggleRoom(ctx, hotel); continue; }
        if (choice == "5") { removeRoom(ctx, hotel); continue; }
        std::cout << "Unknown option.\n";
        pause();
    }
}

void listRestaurants(AppContext& ctx, const Hotel& hotel) {
    banner("Restaurants");
    auto restaurants = ctx.svc.restaurants->listByHotel(hotel.id);
    if (restaurants.empty()) {
        std::cout << "No restaurants configured yet.\n";
        pause();
        return;
    }
    std::sort(restaurants.begin(), restaurants.end(), [](const Restaurant& a, const Restaurant& b) {
        return a.name < b.name;
    });
    for (const auto& restaurant : restaurants) {
        std::cout << "- " << restaurant.name << " (" << restaurant.id << ")"
                  << (restaurant.active ? "" : " [inactive]") << '\n';
        std::cout << "  Cuisine : " << restaurant.cuisine << '\n';
        std::cout << "  Hours   : " << restaurant.openHours << '\n';
        if (restaurant.menu.empty()) {
            std::cout << "  Menu    : (no items)\n";
        } else {
            std::cout << "  Menu    :\n";
            for (const auto& item : restaurant.menu) {
                std::cout << "    - " << item.name
                          << " - " << formatMoney(item.priceCents)
                          << " (" << item.category << ")"
                          << (item.active ? "" : " [inactive]")
                          << '\n';
            }
        }
    }
    pause();
}

void addRestaurant(AppContext& ctx, Hotel& hotel) {
    banner("Add restaurant");
    Restaurant restaurant{};
    restaurant.id = nextRestaurantId(*ctx.svc.restaurants);
    restaurant.hotelId = hotel.id;
    restaurant.name = readLine("Name: ");
    restaurant.cuisine = readLine("Cuisine: ");
    restaurant.openHours = readLine("Hours (e.g., 18:00-23:00): ");
    restaurant.active = true;
    ctx.svc.restaurants->upsert(restaurant);
    ctx.svc.restaurants->saveAll();
    std::cout << "Restaurant created with ID " << restaurant.id << "\n";
    pause();
}

void editRestaurant(AppContext& ctx, Hotel& hotel) {
    banner("Edit restaurant");
    const auto id = readLine("Restaurant ID: ");
    auto restaurantOpt = ctx.svc.restaurants->get(id);
    if (!restaurantOpt || restaurantOpt->hotelId != hotel.id) {
        std::cout << "Restaurant not found for this hotel.\n";
        pause();
        return;
    }
    auto restaurant = *restaurantOpt;
    const auto name = readLine("Name [" + restaurant.name + "]: ", true);
    if (!name.empty()) restaurant.name = name;
    const auto cuisine = readLine("Cuisine [" + restaurant.cuisine + "]: ", true);
    if (!cuisine.empty()) restaurant.cuisine = cuisine;
    const auto hours = readLine("Hours [" + restaurant.openHours + "]: ", true);
    if (!hours.empty()) restaurant.openHours = hours;
    ctx.svc.restaurants->upsert(restaurant);
    ctx.svc.restaurants->saveAll();
    std::cout << "Restaurant updated.\n";
    pause();
}

void toggleRestaurant(AppContext& ctx, Hotel& hotel) {
    banner("Toggle restaurant");
    const auto id = readLine("Restaurant ID: ");
    auto restaurantOpt = ctx.svc.restaurants->get(id);
    if (!restaurantOpt || restaurantOpt->hotelId != hotel.id) {
        std::cout << "Restaurant not found for this hotel.\n";
        pause();
        return;
    }
    auto restaurant = *restaurantOpt;
    restaurant.active = !restaurant.active;
    ctx.svc.restaurants->upsert(restaurant);
    ctx.svc.restaurants->saveAll();
    std::cout << "Restaurant " << (restaurant.active ? "activated" : "deactivated") << ".\n";
    pause();
}

void removeRestaurant(AppContext& ctx, Hotel& hotel) {
    banner("Remove restaurant");
    const auto id = readLine("Restaurant ID: ");
    auto restaurantOpt = ctx.svc.restaurants->get(id);
    if (!restaurantOpt || restaurantOpt->hotelId != hotel.id) {
        std::cout << "Restaurant not found for this hotel.\n";
        pause();
        return;
    }
    if (!restaurantOpt->menu.empty()) {
        std::cout << "Clear the menu items before removing a restaurant.\n";
        pause();
        return;
    }
    ctx.svc.restaurants->remove(id);
    ctx.svc.restaurants->saveAll();
    std::cout << "Restaurant removed.\n";
    pause();
}

void manageMenu(AppContext& ctx, Hotel& hotel) {
    banner("Manage menu");
    const auto id = readLine("Restaurant ID: ");
    auto restaurantOpt = ctx.svc.restaurants->get(id);
    if (!restaurantOpt || restaurantOpt->hotelId != hotel.id) {
        std::cout << "Restaurant not found for this hotel.\n";
        pause();
        return;
    }
    auto restaurant = *restaurantOpt;
    for (;;) {
        banner("Menu for " + restaurant.name);
        if (restaurant.menu.empty()) {
            std::cout << "No items yet.\n";
        } else {
            std::cout << std::left
                      << std::setw(5)  << "#"
                      << std::setw(20) << "Name"
                      << std::setw(12) << "Category"
                      << std::setw(12) << "Price"
                      << "Status" << '\n';
            std::cout << std::string(60, '-') << '\n';
            for (std::size_t i = 0; i < restaurant.menu.size(); ++i) {
                const auto& item = restaurant.menu[i];
                std::cout << std::left
                          << std::setw(5)  << (i + 1)
                          << std::setw(20) << item.name.substr(0, 19)
                          << std::setw(12) << item.category.substr(0, 11)
                          << std::setw(12) << formatMoney(item.priceCents)
                          << (item.active ? "Active" : "Inactive")
                          << '\n';
            }
        }
        std::cout << "\n1) Add item\n";
        std::cout << "2) Edit item\n";
        std::cout << "3) Toggle availability\n";
        std::cout << "4) Remove item\n";
        std::cout << "0) Back\n";
        const auto choice = readLine("Select: ");
        if (choice == "0") {
            ctx.svc.restaurants->upsert(restaurant);
            ctx.svc.restaurants->saveAll();
            return;
        }
        if (choice == "1") {
            MenuItem item{};
            item.id = nextMenuItemId(restaurant);
            item.restaurantId = restaurant.id;
            item.name = readLine("Item name: ");
            item.category = readLine("Category: ");
            for (;;) {
                const auto price = readLine("Price (EUR): ");
                if (const auto parsed = parseMoney(price)) {
                    item.priceCents = std::max<std::int64_t>(0, *parsed);
                    break;
                }
                std::cout << "Enter an amount such as 650 or 199.50.\n";
            }
            item.active = true;
            restaurant.menu.push_back(item);
            std::cout << "Item added.\n";
            pause();
            continue;
        }
        if (choice == "2") {
            const auto number = readLine("Item number: ");
            if (const auto parsed = parseInt(number)) {
                if (*parsed >= 1 && static_cast<std::size_t>(*parsed) <= restaurant.menu.size()) {
                    auto& item = restaurant.menu[static_cast<std::size_t>(*parsed) - 1];
                    const auto name = readLine("Name [" + item.name + "]: ", true);
                    if (!name.empty()) item.name = name;
                    const auto category = readLine("Category [" + item.category + "]: ", true);
                    if (!category.empty()) item.category = category;
                    const auto price = readLine("Price [" + formatMoney(item.priceCents) + "]: ", true);
                    if (!price.empty()) {
                        if (const auto parsedMoney = parseMoney(price)) {
                            item.priceCents = std::max<std::int64_t>(0, *parsedMoney);
                        }
                    }
                    std::cout << "Item updated.\n";
                    pause();
                    continue;
                }
            }
            std::cout << "Invalid selection.\n";
            pause();
            continue;
        }
        if (choice == "3") {
            const auto number = readLine("Item number: ");
            if (const auto parsed = parseInt(number)) {
                if (*parsed >= 1 && static_cast<std::size_t>(*parsed) <= restaurant.menu.size()) {
                    auto& item = restaurant.menu[static_cast<std::size_t>(*parsed) - 1];
                    item.active = !item.active;
                    std::cout << "Item " << (item.active ? "activated" : "deactivated") << ".\n";
                    pause();
                    continue;
                }
            }
            std::cout << "Invalid selection.\n";
            pause();
            continue;
        }
        if (choice == "4") {
            const auto number = readLine("Item number: ");
            if (const auto parsed = parseInt(number)) {
                if (*parsed >= 1 && static_cast<std::size_t>(*parsed) <= restaurant.menu.size()) {
                    restaurant.menu.erase(restaurant.menu.begin() + (static_cast<std::size_t>(*parsed) - 1));
                    std::cout << "Item removed.\n";
                    pause();
                    continue;
                }
            }
            std::cout << "Invalid selection.\n";
            pause();
            continue;
        }
        std::cout << "Unknown option.\n";
        pause();
    }
}

void manageRestaurants(AppContext& ctx, Hotel& hotel) {
    for (;;) {
        banner("Manage restaurants");
        std::cout << "1) List restaurants\n";
        std::cout << "2) Add restaurant\n";
        std::cout << "3) Edit restaurant\n";
        std::cout << "4) Toggle restaurant\n";
        std::cout << "5) Remove restaurant\n";
        std::cout << "6) Manage menu\n";
        std::cout << "0) Back\n";
        const auto choice = readLine("Select: ");
        if (choice == "0") return;
        if (choice == "1") { listRestaurants(ctx, hotel); continue; }
        if (choice == "2") { addRestaurant(ctx, hotel); continue; }
        if (choice == "3") { editRestaurant(ctx, hotel); continue; }
        if (choice == "4") { toggleRestaurant(ctx, hotel); continue; }
        if (choice == "5") { removeRestaurant(ctx, hotel); continue; }
        if (choice == "6") { manageMenu(ctx, hotel); continue; }
        std::cout << "Unknown option.\n";
        pause();
    }
}

void reviewBookings(AppContext& ctx, const Hotel& hotel) {
    auto bookings = ctx.svc.bookings->listByHotel(hotel.id);
    if (bookings.empty()) {
        banner("Bookings");
        std::cout << "No bookings yet.\n";
        pause();
        return;
    }
    std::sort(bookings.begin(), bookings.end(), [](const Booking& a, const Booking& b) {
        return a.createdAt > b.createdAt;
    });
    banner("Bookings");
    std::cout << std::left
              << std::setw(12) << "Booking"
              << std::setw(12) << "Status"
              << std::setw(12) << "Rooms"
              << std::setw(12) << "Nights"
              << std::setw(14) << "Room Rev"
              << std::setw(14) << "Dining Rev"
              << "Created" << '\n';
    std::cout << std::string(100, '-') << '\n';
    for (const auto& booking : bookings) {
        const auto metrics = summarize(booking);
        std::tm tm{};
        std::time_t t = static_cast<std::time_t>(booking.createdAt);
#if defined(_WIN32)
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif
        std::ostringstream when;
        when << std::put_time(&tm, "%Y-%m-%d %H:%M");
        std::cout << std::left
                  << std::setw(12) << booking.bookingId
                  << std::setw(12) << bookingStatusToString(booking.status)
                  << std::setw(12) << metrics.rooms
                  << std::setw(12) << metrics.roomNights
                  << std::setw(14) << formatMoney(metrics.roomRevenue)
                  << std::setw(14) << formatMoney(metrics.diningRevenue)
                  << when.str() << '\n';
    }
    pause();
}

void editHotelProfile(AppContext& ctx, Hotel& hotel) {
    banner("Edit hotel profile");
    auto current = hotel;
    const auto name = readLine("Name [" + hotel.name + "]: ", true);
    if (!name.empty()) current.name = name;
    const auto address = readLine("Address [" + hotel.address + "]: ", true);
    if (!address.empty()) current.address = address;
    const auto stars = readLine("Stars (1-5) [" + std::to_string(static_cast<int>(hotel.stars)) + "]: ", true);
    if (!stars.empty()) {
        if (const auto parsed = parseInt(stars); parsed && *parsed >= 1 && *parsed <= 5) {
            current.stars = static_cast<std::uint8_t>(*parsed);
        }
    }
    const auto baseRate = readLine("Base nightly rate [" + (hotel.baseRateCents > 0 ? formatMoney(hotel.baseRateCents) : std::string("n/a")) + "]: ", true);
    if (!baseRate.empty()) {
        if (const auto parsed = parseMoney(baseRate)) {
            current.baseRateCents = std::max<std::int64_t>(0, *parsed);
        }
    }
    ctx.svc.hotels->upsert(current);
    ctx.svc.hotels->saveAll();
    hotel = current;
    std::cout << "Hotel updated.\n";
    pause();
}

} // namespace

namespace hms::ui {

bool DashboardHotelManager(hms::AppContext& ctx) {
    for (;;) {
        auto hotelOpt = managedHotel(ctx);
        if (!hotelOpt) {
            banner("Manager dashboard");
            std::cout << "No hotel has been assigned to your account yet. Please contact an administrator.\n";
            pause();
            return true;
        }
        auto hotel = *hotelOpt;
        banner(hotel.name + " manager");
        std::cout << "1) View snapshot\n";
        std::cout << "2) Edit hotel profile\n";
        std::cout << "3) Manage rooms\n";
        std::cout << "4) Manage restaurants & menus\n";
        std::cout << "5) Review bookings\n";
        std::cout << "0) Log out\n";
        const auto choice = readLine("Select: ");
        if (choice == "0") {
            return true;
        }
        if (choice == "1") { showSummary(ctx, hotel); continue; }
        if (choice == "2") { editHotelProfile(ctx, hotel); continue; }
        if (choice == "3") { manageRooms(ctx, hotel); continue; }
        if (choice == "4") { manageRestaurants(ctx, hotel); continue; }
        if (choice == "5") { reviewBookings(ctx, hotel); continue; }
        std::cout << "Unknown option.\n";
        pause();
    }
}

} // namespace hms::ui
