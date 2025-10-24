#pragma once
#include <string>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace hms {

struct RoomType {
    std::string   id;                // e.g. "RT-DELUXE" (stable key)
    std::string   name;              // e.g. "Deluxe"
    std::int64_t  nightlyRateCents;
    bool          active{true};
};

// JSON (de)serialization
inline void to_json(nlohmann::json& j, const RoomType& rt) {
    j = nlohmann::json{
        {"id", rt.id},
        {"name", rt.name},
        {"nightlyRateCents", rt.nightlyRateCents},
        {"active", rt.active}
    };
}
inline void from_json(const nlohmann::json& j, RoomType& rt) {
    j.at("id").get_to(rt.id);
    j.at("name").get_to(rt.name);
    j.at("nightlyRateCents").get_to(rt.nightlyRateCents);
    // "active" optional for backward compat
    if (j.contains("active")) j.at("active").get_to(rt.active);
}

}
