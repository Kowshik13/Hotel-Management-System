#include "UserRepository.h"

#include <algorithm>
#include <fstream>
#include <filesystem>
#include <system_error>
#include <utility>

#include <nlohmann/json.hpp>

#include "../security/Security.h"

using nlohmann::json;
namespace fs = std::filesystem;

namespace {

fs::path baseDataDir() {
#ifdef HMS_DATA_DIR
    return fs::path{HMS_DATA_DIR};
#else
    return (fs::current_path() / "data").lexically_normal();
#endif
}

bool ensureParentDir(const fs::path& p) {
    try {
        auto dir = p.parent_path();
        if (!dir.empty() && !fs::exists(dir)) {
            return fs::create_directories(dir);
        }
        return true;
    }
    catch (...) {
        return false;
    }
}

std::string roleToString(hms::Role r) {
    switch (r) {
        case hms::Role::ADMIN: return "ADMIN";
        case hms::Role::HOTEL_MANAGER: return "HOTEL_MANAGER";
        case hms::Role::GUEST: return "GUEST";
    }
    return "GUEST";
}

hms::Role roleFromString(const std::string& s) {
    if (s == "ADMIN") return hms::Role::ADMIN;
    if (s == "HOTEL_MANAGER") return hms::Role::HOTEL_MANAGER;
    return hms::Role::GUEST;
}

json toJson(const hms::User& u) {
    return json{
        {"userId",       u.userId},
        {"firstName",    u.firstName},
        {"lastName",     u.lastName},
        {"address",      u.address},
        {"phone",        u.phone},
        {"login",        u.login},
        {"passwordHash", u.passwordHash},
        {"role",         roleToString(u.role)},
        {"active",       u.active}
    };
}

bool fromJson(const json& j, hms::User& u) {
    try {
        u.userId       = j.value("userId", "");
        u.firstName    = j.value("firstName", "");
        u.lastName     = j.value("lastName",  "");
        u.address      = j.value("address",   "");
        u.phone        = j.value("phone",     "");
        u.login        = j.value("login",     "");
        u.passwordHash = j.value("passwordHash", "");
        if (u.passwordHash.empty()) {
            const auto legacy = j.value("password", std::string{});
            if (!legacy.empty()) {
                u.passwordHash = hms::HashPasswordDemo(legacy);
            }
        }
        u.role   = roleFromString(j.value("role", "GUEST"));
        u.active = j.value("active", true);
        u.password.clear();
        return true;
    }
    catch (...) {
        return false;
    }
}

} // namespace

namespace hms {

UserRepository::path_t UserRepository::defaultUsersPath() {
    return (baseDataDir() / "users.json").lexically_normal();
}

UserRepository::UserRepository(path_t path)
    : path_(std::move(path)) {}

bool UserRepository::load() {
    items_.clear();
    if (!ensureParentDir(path_)) return false;

    if (!fs::exists(path_)) {
        std::ofstream out(path_, std::ios::trunc);
        if (!out.good()) return false;
        out << "[]\n";
        return true;
    }

    std::ifstream in(path_);
    if (!in.good()) return false;

    json doc;
    try { in >> doc; }
    catch (...) { return false; }

    if (doc.is_null()) return true;
    if (!doc.is_array()) return false;

    for (const auto& j : doc) {
        User u{};
        if (fromJson(j, u)) {
            items_.push_back(std::move(u));
        }
    }
    return true;
}

bool UserRepository::saveAll() const {
    if (!ensureParentDir(path_)) return false;

    const fs::path tmp = path_.string() + ".tmp";
    json arr = json::array();
    for (const auto& u : items_) arr.push_back(toJson(u));

    {
        std::ofstream out(tmp, std::ios::trunc);
        if (!out.good()) return false;
        out << arr.dump(2) << '\n';
        out.flush();
        if (!out.good()) return false;
    }

    std::error_code ec;
    fs::rename(tmp, path_, ec);
    if (ec) {
        fs::remove(tmp);
        return false;
    }
    return true;
}

std::optional<User> UserRepository::getById(const std::string& userId) const {
    auto it = std::find_if(items_.begin(), items_.end(),
                           [&](const User& u){ return u.userId == userId; });
    if (it == items_.end()) return std::nullopt;
    return *it;
}

std::optional<User> UserRepository::getByLogin(const std::string& login) const {
    auto it = std::find_if(items_.begin(), items_.end(),
                           [&](const User& u){ return u.login == login; });
    if (it == items_.end()) return std::nullopt;
    return *it;
}

std::vector<User> UserRepository::list() const {
    return items_;
}

bool UserRepository::loginTakenByOther(const std::string& login, const std::string& thisUserId) const {
    if (login.empty()) return false;
    for (const auto& u : items_) {
        if (u.login == login && u.userId != thisUserId) return true;
    }
    return false;
}

bool UserRepository::upsert(const User& u) {
    if (u.userId.empty()) return false;
    if (loginTakenByOther(u.login, u.userId)) return false;

    auto it = std::find_if(items_.begin(), items_.end(),
                           [&](const User& x){ return x.userId == u.userId; });
    if (it == items_.end()) items_.push_back(u);
    else                    *it = u;
    return true;
}

bool UserRepository::removeById(const std::string& userId) {
    auto it = std::remove_if(items_.begin(), items_.end(),
                             [&](const User& x){ return x.userId == userId; });
    if (it == items_.end()) return false;
    items_.erase(it, items_.end());
    return true;
}

bool UserRepository::removeByLogin(const std::string& login) {
    auto it = std::remove_if(items_.begin(), items_.end(),
                             [&](const User& x){ return x.login == login; });
    if (it == items_.end()) return false;
    items_.erase(it, items_.end());
    return true;
}

} // namespace hms
