#pragma once
#include <string>
#include <cstdint>

namespace hms {

    struct MenuItem {
        std::string id;         // "bf-omelette"
        std::string name;       // "Masala Omelette"
        std::string category;   // "Breakfast" | "Lunch" | "Dinner" | ...
        std::int64_t price{};
        bool active{true};
    };

} // namespace hms
