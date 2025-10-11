#pragma once
#include <string>
#include <cstdint>

namespace hms {
    struct RestaurantOrderLine {
        std::string  lineId;              // "ROL-0001" unique within the booking
        std::string  restaurantId;        // which restaurant fulfilled it
        std::string  category;            // snapshot (e.g., "Breakfast")
        std::string  menuItemId;          // MenuItem.id
        std::string  nameSnapshot;        // MenuItem.name at order time
        std::int64_t unitPriceSnapshot{}; // price at order time (smallest unit)
        int          qty{1};
        int          billedRoomNumber{0}; // link to a room in this booking (0=none)
        std::string  takenByUsername;     // staff username (who took the order)
        std::string  orderedByGuestId;    // optional: which occupant ordered ("" if N/A)
        std::int64_t createdAt{0};        // epoch seconds
    };
}