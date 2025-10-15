#include "LoginScreen.h"
#include "../core/ConsoleIO.h"
#include "../../security/Security.h"
#include <iostream>

namespace hms::ui {
    std::optional<hms::User> LoginScreen(hms::UserRepository& users) {
        banner("Login");
        for(int i=0;i<3;++i) {
            auto login = readLine("Login: ");
            auto pw    = readPassword("Password: ");
            auto u = users.getByLogin(login);
            if (u && hms::VerifyPasswordDemo(u->passwordHash, pw)) {
                auto sanitized = *u;
                sanitized.password.clear();
                return sanitized;
            }
            std::cout << "Invalid login or password. "<<(2-i)<<" tries left.\n";
        }
        return std::nullopt;
    }
}

