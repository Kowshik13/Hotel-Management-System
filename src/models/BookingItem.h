#pragma once
#include <variant>
#include <string>
#include <stdexcept>
#include <nlohmann/json.hpp>

#include "RoomStayItem.h"
#include "RestaurantOrderLine.h"
#include "BookingType.h"

namespace hms {

    // Variant of the two possible booking line types
    using BookingItem = std::variant<RoomStayItem, RestaurantOrderLine>;

    // Helper (kept for callers)
    inline BookingType kindOf(const BookingItem& it) {
        if (std::holds_alternative<RoomStayItem>(it))        return BookingType::RoomStayItem;
        if (std::holds_alternative<RestaurantOrderLine>(it)) return BookingType::RestaurantOrder;
        return BookingType::RoomStayItem;
    }

    // ---------- JSON (de)serialization for BookingItem ----------
    // We serialize as an object: { "kind": "...", "value": { ... } }
    inline void to_json(nlohmann::json& j, const BookingItem& bi) {
        if (std::holds_alternative<RoomStayItem>(bi)) {
            j = nlohmann::json{
                {"kind",  "RoomStayItem"},
                {"value", std::get<RoomStayItem>(bi)}
            };
        }
        else {
            j = nlohmann::json{
                {"kind",  "RestaurantOrder"},
                {"value", std::get<RestaurantOrderLine>(bi)}
            };
        }
    }

    inline void from_json(const nlohmann::json& j, BookingItem& bi) {
        const std::string kind = j.at("kind").get<std::string>();
        if (kind == "RoomStayItem") {
            bi = j.at("value").get<RoomStayItem>();
        }
        else if (kind == "RestaurantOrder") {
            bi = j.at("value").get<RestaurantOrderLine>();
        }
        else {
            throw std::runtime_error("Unknown BookingItem kind: " + kind);
        }
    }

}
