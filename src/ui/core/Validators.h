#pragma once
#include <string>
#include "../../storage/UserRepository.h"

namespace hms::ui {
    bool validateLogin(const hms::UserRepository& repo, const std::string& login, std::string& why);
    bool validatePasswordStrength(const std::string& pw, std::string& why);
}
