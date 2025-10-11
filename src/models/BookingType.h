#pragma once
#include <string>

namespace hms {
    enum class BookingType { RoomStayItem, RestaurantOrder };
    inline std::string to_string(BookingType k) {
        switch (k) {
            case BookingType::RoomStayItem:    return "RoomStayItem";
            case BookingType::RestaurantOrder: return "RestaurantOrder";
        }
        return "RoomStayItem";
    }
}
