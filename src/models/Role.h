#pragma once
#include <nlohmann/json.hpp>

namespace hms {

enum class Role {
    GUEST,
    MANAGER,
    ADMIN
};

// JSON serialization mapping
NLOHMANN_JSON_SERIALIZE_ENUM(Role, {
    {Role::GUEST, "GUEST"},
    {Role::MANAGER, "MANAGER"},
    {Role::ADMIN, "ADMIN"}
})

}