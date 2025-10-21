#include "Security.h"

#include <cstdint>
#include <iomanip>
#include <sstream>

namespace {
    std::string fnv1a64(const std::string& input) {
        constexpr std::uint64_t offset = 1469598103934665603ULL;
        constexpr std::uint64_t prime  = 1099511628211ULL;

        std::uint64_t hash = offset;
        for (unsigned char c : input) {
            hash ^= static_cast<std::uint64_t>(c);
            hash *= prime;
        }

        std::ostringstream oss;
        oss << std::hex << std::setw(16) << std::setfill('0') << hash;
        return oss.str();
    }
}

namespace hms {

std::string HashPasswordDemo(const std::string& password) {
    return fnv1a64(password);
}

bool VerifyPasswordDemo(const std::string& passwordHash, const std::string& candidate) {
    return passwordHash == HashPasswordDemo(candidate);
}

}
