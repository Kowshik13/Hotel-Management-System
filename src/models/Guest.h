#pragma once
#include <string>
#include <cstdint>
#include "Role.h"

namespace hms {
    struct Guest {
        std::string firstName;
        std::string lastName;
        std::string address;
        std::string phone;

        std::string login;
        std::string password;

        Role role{Role::USER};
    };
}

