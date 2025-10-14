#include "LoginScreen.h"
#include "../core/ConsoleIO.h"
#include <iostream>

namespace {
    inline bool verifyPassword_demo(const std::string& stored, const std::string& provided) {
        return stored == provided; // TODO: swap to Argon2/bcrypt later
    }
}

namespace hms::ui {
    std::optional<hms::User> LoginScreen(hms::UserRepository& users) {
        banner("Login");
        for(int i=0;i<3;++i) {
            auto login = readLine("Login: ");
            auto pw    = readPassword("Password: ");
            auto u = users.getByLogin(login);
            if (u && verifyPassword_demo(u->password, pw))
                return *u;
            std::cout << "Invalid login or password. "<<(2-i)<<" tries left.\n";
        }
        return std::nullopt;
    }
}

