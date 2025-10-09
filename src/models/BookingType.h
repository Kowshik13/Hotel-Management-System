#pragma once
#include <string>

namespace hms {
    enum class BookingType {
        RoomStay,
        RestaurantOrder
    };

    inline std::string to_string(BookingType k) {
        switch (k) {
            case BookingType::RoomStay:        return "RoomStay";
            case BookingType::RestaurantOrder: return "RestaurantOrder";
        }
        return "RoomStay";
    }
}
