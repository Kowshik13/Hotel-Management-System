#pragma once
#include <string>
#include <vector>

#include "MenuItem.h"

namespace hms {
    struct Restaurant {
        std::string id;
        std::string hotelId;
        std::string name;
        std::string cuisine;
        std::string openHours;
        bool active{true};
        std::vector<MenuItem> menu;
    };
}
