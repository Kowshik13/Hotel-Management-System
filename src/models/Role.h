#pragma once
#include <string>

namespace hms {
    enum class Role { GUEST, ADMIN };

    inline std::string to_string(Role r) {
        switch (r) {
            case Role::GUEST:  return "GUEST";
            case Role::ADMIN: return "ADMIN";
        }
        return "GUEST";
    }
}
