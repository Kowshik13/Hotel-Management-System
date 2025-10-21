#pragma once
#include <string>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace hms {

struct RestaurantOrderLine {
    std::string  lineId;              // unique within booking, e.g. "ROL-0001"
    std::string  restaurantId;        // which restaurant fulfilled the order
    std::string  category;            // e.g. "Breakfast", "Dinner"
    std::string  menuItemId;          // MenuItem.id
    std::string  nameSnapshot;        // MenuItem.name at order time
    std::int64_t unitPriceSnapshot{}; // price at order time (in cents)
    int          qty{1};
    int          billedRoomNumber{0}; // link to booked room; 0 if none
    std::string  takenByUsername;     // staff user who took order
    std::string  orderedByGuestId;    // optional: guest who ordered
    std::int64_t createdAt{0};        // epoch seconds
};

// JSON serialization helpers
inline void to_json(nlohmann::json& j, const RestaurantOrderLine& rol) {
    j = nlohmann::json{
        {"lineId", rol.lineId},
        {"restaurantId", rol.restaurantId},
        {"category", rol.category},
        {"menuItemId", rol.menuItemId},
        {"nameSnapshot", rol.nameSnapshot},
        {"unitPriceSnapshot", rol.unitPriceSnapshot},
        {"qty", rol.qty},
        {"billedRoomNumber", rol.billedRoomNumber},
        {"takenByUsername", rol.takenByUsername},
        {"orderedByGuestId", rol.orderedByGuestId},
        {"createdAt", rol.createdAt}
    };
}

inline void from_json(const nlohmann::json& j, RestaurantOrderLine& rol) {
    j.at("lineId").get_to(rol.lineId);
    j.at("restaurantId").get_to(rol.restaurantId);
    j.at("category").get_to(rol.category);
    j.at("menuItemId").get_to(rol.menuItemId);
    j.at("nameSnapshot").get_to(rol.nameSnapshot);
    j.at("unitPriceSnapshot").get_to(rol.unitPriceSnapshot);
    j.at("qty").get_to(rol.qty);
    j.at("billedRoomNumber").get_to(rol.billedRoomNumber);
    j.at("takenByUsername").get_to(rol.takenByUsername);
    j.at("orderedByGuestId").get_to(rol.orderedByGuestId);
    j.at("createdAt").get_to(rol.createdAt);
}

} // namespace hms
