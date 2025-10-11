#pragma once
#include <string>
#include <vector>
#include <variant>
#include <cstdint>
#include "RoomStayItem.h"
#include "RestaurantOrderLine.h"
#include "BookingType.h"

namespace hms {
    enum class BookingStatus { ACTIVE, CHECKED_OUT, CANCELLED };

    using BookingItem = std::variant<RoomStayItem, RestaurantOrderLine>;

    struct Booking {
        std::string   bookingId;
        std::string   hotelId;
        BookingStatus status{BookingStatus::ACTIVE};
        std::int64_t  createdAt{0};
        std::int64_t  updatedAt{0};

        std::string   primaryGuestId;      // points to one occupant guestId

        std::vector<BookingItem> items;    // mix of RoomStayItem + RestaurantOrderLine
    };


    inline BookingType kindOf(const BookingItem& it) {
        if (std::holds_alternative<RoomStayItem>(it))        return BookingType::RoomStayItem;
        if (std::holds_alternative<RestaurantOrderLine>(it)) return BookingType::RestaurantOrder;
        return BookingType::RoomStayItem;
    }
}
