#pragma once
#include <string>
#include <cstdint>
#include <variant>
#include <vector>
#include "Guest.h"
#include "BookingType.h"
#include "RestaurantOrder.h"

namespace hms {

    struct RoomStay {
        int roomNumber{};
        int nights{1};
        std::int64_t nightlyRateLocked{};
        Guest occupants;
    };

    using BookingItem = std::variant<RoomStay, RestaurantOrder>;

    inline BookingType kindOf(const BookingItem &it) {
        if (std::holds_alternative<RoomStay>(it))
            return BookingType::RoomStay;

        if (std::holds_alternative<RestaurantOrder>(it))
            return BookingType::RestaurantOrder;

        return BookingType::RoomStay;
    }
}
