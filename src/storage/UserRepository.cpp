#include "UserRepository.h"

#include <fstream>
#include <algorithm>
#include <cstdio>
#include <utility>
#include <filesystem>
#include "../external/nlohmann/json.hpp"

using nlohmann::json;
namespace fs = std::filesystem;

namespace hms {


    UserRepository::path_t UserRepository::defaultUsersPath() {
        fs::path target = fs::current_path() / ".." / "src" / "data" / "users.json";
        return target.lexically_normal();
    }

    UserRepository::UserRepository(path_t path)
            : path_(std::move(path)) {}

    static bool ensureParentDir(const fs::path& p) {
        try {
            auto dir = p.parent_path();
            if (!dir.empty() && !fs::exists(dir)) return fs::create_directories(dir);
            return true;
        } catch (...) { return false; }
    }


    static std::string roleToString(Role r) {
        switch (r) {
            case Role::ADMIN: return "ADMIN";
            case Role::GUEST: return "GUEST";
        }
        return "GUEST";
    }

    static Role roleFromString(const std::string& s) {
        if (s == "ADMIN") return Role::ADMIN;
        return Role::GUEST;
    }


    static json toJson(const User& u) {
        return json{
                {"userId",    u.userId},
                {"firstName", u.firstName},
                {"lastName",  u.lastName},
                {"address",   u.address},
                {"phone",     u.phone},
                {"login",     u.login},
                {"password",  u.password},   // plaintext in your current model
                {"role",      roleToString(u.role)}
        };
    }

    static bool fromJson(const json& j, User& u) {
        try {
            u.userId    = j.value("userId", "");
            u.firstName = j.value("firstName", "");
            u.lastName  = j.value("lastName",  "");
            u.address   = j.value("address",   "");
            u.phone     = j.value("phone",     "");
            u.login     = j.value("login",     "");
            u.password  = j.value("password",  "");
            u.role      = roleFromString(j.value("role", "GUEST"));
            return true;
        } catch (...) {
            return false;
        }
    }


    bool UserRepository::load() {
        items_.clear();
        if (!ensureParentDir(path_)) return false;

        if (!fs::exists(path_)) {
            std::ofstream out(path_, std::ios::trunc);
            if (!out.good()) return false;
            out << "[]";               // empty array
            return true;
        }

        std::ifstream in(path_);
        if (!in.good()) return false;

        json doc;
        try { in >> doc; } catch (...) { return false; }
        if (!doc.is_array()) return false;

        for (const auto& j : doc) {
            User u{};
            if (fromJson(j, u)) items_.push_back(std::move(u));
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
            out << arr.dump(2);
            out.flush();
            if (!out.good()) return false;
        }
        if (std::rename(tmp.string().c_str(), path_.string().c_str()) != 0) {
            std::remove(tmp.string().c_str());
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
        return items_; // copy
    }


    bool UserRepository::loginTakenByOther(const std::string& login, const std::string& thisUserId) const {
        if (login.empty()) return false;
        for (const auto& u : items_) {
            if (u.login == login && u.userId != thisUserId) return true;
        }
        return false;
    }

    bool UserRepository::upsert(const User& u) {
        // Require a userId; ID generation should happen in a service layer.
        if (u.userId.empty()) return false;

        // Enforce unique login (except for the same userId)
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

}
