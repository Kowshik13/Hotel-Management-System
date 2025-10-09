#pragma once
#include <string>
#include <cstdint>

namespace hms {

    // One menu item ordered on a booking.
    struct RestaurantOrder {
        std::string  orderId;

        // Snapshot from MenuItem at order time
        std::string  category;            // copy of MenuItem.category
        std::string  menuItemId;          // MenuItem.id
        std::string  nameSnapshot;        // MenuItem.name
        std::int64_t unitPriceSnapshot{}; // MenuItem.price at order time
    };

} // namespace hms
