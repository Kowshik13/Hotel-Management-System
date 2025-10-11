#pragma once
#include <string>

namespace hms {
    struct Restaurant {
        std::string id;
        std::string hotelId;
        std::string name;
        std::string cuisine;
        std::string openHours;
        bool active{true};
    };
}