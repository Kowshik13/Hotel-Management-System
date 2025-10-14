#pragma once
#include "../../storage/UserRepository.h"
#include "../../models/User.h"
#include <optional>

namespace hms::ui {
    std::optional<hms::User> LoginScreen(hms::UserRepository& users);
}
