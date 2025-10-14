#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <nlohmann/json.hpp>
#include "User.h"

namespace hms {

struct RoomStayItem {
    std::string   hotelId;              // which hotel this stay belongs to
    int           roomNumber{};         // numeric room label
    int           nights{1};            // total nights booked
    std::int64_t  nightlyRateLocked{};  // rate at time of booking (in cents)
    std::vector<User> occupants;        // guests staying in the room
};

// JSON serialization helpers
inline void to_json(nlohmann::json& j, const RoomStayItem& rsi) {
    j = nlohmann::json{
        {"hotelId", rsi.hotelId},
        {"roomNumber", rsi.roomNumber},
        {"nights", rsi.nights},
        {"nightlyRateLocked", rsi.nightlyRateLocked},
        {"occupants", rsi.occupants}
    };
}

inline void from_json(const nlohmann::json& j, RoomStayItem& rsi) {
    j.at("hotelId").get_to(rsi.hotelId);
    j.at("roomNumber").get_to(rsi.roomNumber);
    j.at("nights").get_to(rsi.nights);
    j.at("nightlyRateLocked").get_to(rsi.nightlyRateLocked);
    if (j.contains("occupants")) j.at("occupants").get_to(rsi.occupants);
}

} // namespace hms
