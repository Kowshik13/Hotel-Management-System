#pragma once
#include <string>

namespace hms {

// NOTE: These helpers are intentionally simple and deterministic so that
// credentials stored in fixture JSON files keep working across platforms.
// Swap them out for Argon2/bcrypt/Scrypt in production.
std::string HashPasswordDemo(const std::string& password);
bool        VerifyPasswordDemo(const std::string& passwordHash, const std::string& candidate);

}
