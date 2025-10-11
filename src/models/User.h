#pragma once
#include <string>
#include <cstdint>
#include "Role.h"

namespace hms {
    struct User {
        std::string userId;
        std::string firstName;
        std::string lastName;
        std::string address;
        std::string phone;

        std::string login;
        std::string password;

        Role role{Role::GUEST};
    };
}

