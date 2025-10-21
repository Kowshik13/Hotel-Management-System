#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <nlohmann/json.hpp>

#include "BookingStatus.h"
#include "BookingItem.h"  // brings BookingItem + its JSON support

namespace hms {

    struct Booking {
        std::string   bookingId;       // unique ID for the booking
        std::string   hotelId;         // foreign key to hotel
        BookingStatus status{ BookingStatus::ACTIVE };

        std::int64_t  createdAt{ 0 };    // epoch seconds (UTC)
        std::int64_t  updatedAt{ 0 };

        std::string   primaryGuestId;  // references User/Guest.id

        std::vector<BookingItem> items; // list of room stays and restaurant orders
    };

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

}
