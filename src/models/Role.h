#pragma once
#include <nlohmann/json.hpp>

namespace hms {

enum class Role {
    GUEST,
    HOTEL_MANAGER,
    ADMIN
};

// JSON serialization mapping
NLOHMANN_JSON_SERIALIZE_ENUM(Role, {
    {Role::GUEST, "GUEST"},
    {Role::HOTEL_MANAGER, "HOTEL_MANAGER"},
    {Role::ADMIN, "ADMIN"}
})

} // namespace hms
