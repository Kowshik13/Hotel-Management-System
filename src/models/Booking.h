#pragma once
#include <string>
#include <vector>
#include <variant>
#include <cstdint>
#include <nlohmann/json.hpp>
#include "RoomStayItem.h"
#include "RestaurantOrderLine.h"
#include "BookingType.h"

namespace hms {

enum class BookingStatus { ACTIVE, CHECKED_OUT, CANCELLED };

// map enum <-> string for JSON
NLOHMANN_JSON_SERIALIZE_ENUM(BookingStatus, {
    {BookingStatus::ACTIVE, "ACTIVE"},
    {BookingStatus::CHECKED_OUT, "CHECKED_OUT"},
    {BookingStatus::CANCELLED, "CANCELLED"}
});

using BookingItem = std::variant<RoomStayItem, RestaurantOrderLine>;

struct Booking {
    std::string   bookingId;       // unique ID for the booking
    std::string   hotelId;         // foreign key to hotel
    BookingStatus status{BookingStatus::ACTIVE};

    std::int64_t  createdAt{0};    // epoch seconds (UTC)
    std::int64_t  updatedAt{0};

    std::string   primaryGuestId;  // references User/Guest.id

    std::vector<BookingItem> items; // list of room stays and restaurant orders
};

// helpers
inline BookingType kindOf(const BookingItem& it) {
    if (std::holds_alternative<RoomStayItem>(it))        return BookingType::RoomStayItem;
    if (std::holds_alternative<RestaurantOrderLine>(it)) return BookingType::RestaurantOrder;
    return BookingType::RoomStayItem;
}

// JSON (de)serialization
inline void to_json(nlohmann::json& j, const Booking& b) {
    j = nlohmann::json{
        {"bookingId", b.bookingId},
        {"hotelId", b.hotelId},
        {"status", b.status},
        {"createdAt", b.createdAt},
        {"updatedAt", b.updatedAt},
        {"primaryGuestId", b.primaryGuestId},
        {"items", b.items}
    };
}

inline void from_json(const nlohmann::json& j, Booking& b) {
    j.at("bookingId").get_to(b.bookingId);
    j.at("hotelId").get_to(b.hotelId);
    j.at("status").get_to(b.status);
    j.at("createdAt").get_to(b.createdAt);
    j.at("updatedAt").get_to(b.updatedAt);
    j.at("primaryGuestId").get_to(b.primaryGuestId);
    if (j.contains("items")) j.at("items").get_to(b.items);
}

} // namespace hms
