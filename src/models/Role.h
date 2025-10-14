#pragma once
#include <nlohmann/json.hpp>

namespace hms {

enum class Role {
    GUEST,
    ADMIN
};

// JSON serialization mapping
NLOHMANN_JSON_SERIALIZE_ENUM(Role, {
    {Role::GUEST, "GUEST"},
    {Role::ADMIN, "ADMIN"}
})

} // namespace hms
