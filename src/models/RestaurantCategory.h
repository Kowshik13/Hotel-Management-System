#pragma once
#include <string>

namespace hms {
    enum class RestaurantCategory { Breakfast, Lunch, Dinner };

    inline std::string to_string(RestaurantCategory c) {
        switch (c) {
            case RestaurantCategory::Breakfast: return "Breakfast";
            case RestaurantCategory::Lunch:     return "Lunch";
            case RestaurantCategory::Dinner:    return "Dinner";
        }
        return "Breakfast";
    }
}