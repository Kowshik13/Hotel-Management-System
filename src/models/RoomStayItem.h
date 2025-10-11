#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "User.h"

namespace hms {
    struct RoomStayItem {
        std::string   hotelId;
        int           roomNumber{};
        int           nights{1};
        std::int64_t  nightlyRateLocked{}; // snapshot at booking time
        std::vector<User> occupants;
    };
}
