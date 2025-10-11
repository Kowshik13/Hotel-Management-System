#pragma once
#include <string>
#include <vector>

namespace hms {
    struct Room {
        std::string hotelId{"HTL-0001"};   // ← associate room with its hotel
        int         number{0};
        std::string typeId;
        int         sizeSqm{0};
        int         beds{1};
        std::vector<std::string> amenities;
        std::string notes;
        bool        active{true};
    };
}