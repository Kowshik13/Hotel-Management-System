// src/models/User.h
#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include "Role.h"   // ensure Role is defined (see note below)

namespace hms {

struct User {
    std::string userId;       // stable key, e.g., "USR-0001"
    std::string firstName;
    std::string lastName;
    std::string address;
    std::string phone;

    std::string login;

    // SECURITY NOTE:
    // Keep raw password OUT of persisted JSON. Use a hash field instead.
    // We'll still accept "password" on read for backward compatibility.
    std::string password;      // in-memory only (e.g., during registration)
    std::string passwordHash;  // persisted

    Role        role{Role::GUEST};
    bool        active{true};
};

// ---- JSON helpers ----
// We DO NOT serialize the raw 'password'. We persist 'passwordHash' instead.
inline void to_json(nlohmann::json& j, const User& u) {
    j = nlohmann::json{
        {"userId", u.userId},
        {"firstName", u.firstName},
        {"lastName", u.lastName},
        {"address", u.address},
        {"phone", u.phone},
        {"login", u.login},
        {"passwordHash", u.passwordHash},
        {"role", u.role},       // requires Role to be JSON-serializable (see note)
        {"active", u.active}
    };
}

inline void from_json(const nlohmann::json& j, User& u) {
    j.at("userId").get_to(u.userId);
    j.at("firstName").get_to(u.firstName);
    j.at("lastName").get_to(u.lastName);
    if (j.contains("address")) j.at("address").get_to(u.address);
    if (j.contains("phone"))   j.at("phone").get_to(u.phone);
    if (j.contains("login"))   j.at("login").get_to(u.login);

    // backward-compat: accept either passwordHash or legacy plain "password"
    if (j.contains("passwordHash")) j.at("passwordHash").get_to(u.passwordHash);
    else if (j.contains("password")) j.at("password").get_to(u.password); // legacy only; consider migrating to hash

    if (j.contains("role"))   j.at("role").get_to(u.role);
    if (j.contains("active")) j.at("active").get_to(u.active);
}

} // namespace hms
