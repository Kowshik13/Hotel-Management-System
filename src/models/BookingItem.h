#pragma once
#include <string>
#include <cstdint>
#include <variant>
#include <vector>
#include "User.h"
#include "BookingType.h"
#include "RestaurantOrderLine.h"
#include "BookingStatus.h"
#include "RoomStayItem.h"

namespace hms {

    using BookingItem = std::variant<RoomStayItem, RestaurantOrderLine>;

    inline BookingType kindOf(const BookingItem &it) {
        if (std::holds_alternative<RoomStayItem>(it))
            return BookingType::RoomStayItem;

        if (std::holds_alternative<RestaurantOrderLine>(it))
            return BookingType::RestaurantOrder;

        return BookingType::RoomStayItem;
    }
}
