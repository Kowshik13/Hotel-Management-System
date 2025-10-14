#include "RegisterScreen.h"
#include "../core/ConsoleIO.h"
#include "../core/Validators.h"
#include <chrono>
#include <random>
#include <iostream>

namespace {
    std::string genUserId() {
        using namespace std::chrono;
        auto now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        std::mt19937_64 rng(static_cast<uint64_t>(now));
        return "USR-" + std::to_string(now) + "-" + std::to_string(rng() & 0xFFFF);
    }
}

namespace hms::ui {
    std::optional<hms::User> RegisterScreen(hms::UserRepository& users, bool allowAdminBootstrap){
        banner("Create account");
        std::string why;

        std::string login;
        for(;;) {
            login = readLine("Choose a login: ");
            if (validateLogin(users, login, why))
                break;
            std::cout<<"  "<<why<<"\n";
        }

        std::string pw, pw2;
        for(;;) {
            pw  = readPassword("Choose a password: ");
            pw2 = readPassword("Repeat password:   ");
            if(pw!=pw2){ std::cout<<"  Passwords do not match.\n"; continue; }
            if(validatePasswordStrength(pw, why)) break;
            std::cout<<"  "<<why<<"\n";
        }

        // Role selection policy:
        // - Normal: only GUEST can self-register
        // - Bootstrap: if DB has no ADMIN, allow creating one (or allowAdminBootstrap==true)
        hms::Role role = hms::Role::GUEST;
        bool hasAdmin = false;
        for (const auto& u : users.list())
            if (u.role == hms::Role::ADMIN) {
                hasAdmin = true; break;
            }

        if(allowAdminBootstrap && !hasAdmin) {
            auto ans = readLine("Register as ADMIN? (y/N): ", true);
            if (!ans.empty() && (ans=="y"||ans=="Y"))
                role = hms::Role::ADMIN;
        }

        hms::User u{};
        u.userId   = genUserId();
        u.login    = login;
        u.password = pw; // TODO: store hash instead
        u.role     = role;

        u.firstName = readLine("First name (optional): ", true);
        u.lastName  = readLine("Last name  (optional): ", true);
        u.phone     = readLine("Phone      (optional): ", true);
        u.address   = readLine("Address    (optional): ", true);

        if(!users.upsert(u) || !users.saveAll()){
            std::cout<<"Could not save user.\n"; return std::nullopt;
        }
        std::cout<<"Account created ("<<(u.role==hms::Role::ADMIN? "ADMIN" : "GUEST")<<").\n";
        return u;
    }
}
