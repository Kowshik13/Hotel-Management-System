#pragma once
#include <string>
#include <cstdint>

namespace hms {
    struct MenuItem {
        std::string  id;
        std::string  restaurantId;
        std::string  name;         // "Masala Omelette"
        std::string  category;     // "Breakfast" | "Lunch" | "Dinner" | ...
        std::int64_t priceCents{};
        bool         active{true};
    };
}
