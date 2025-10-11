#include "UserRepository.h"
#include <fstream>
#include <algorithm>
#include <utility>
#include "../external/nlohmann/json.hpp"
#include <filesystem>

using nlohmann::json;

namespace fs = std::filesystem;
namespace hms {

    UsersRepository::path_t UsersRepository::defaultUsersPath() {
        fs::path target = fs::current_path() / ".." / "src" / "data" / "users.json";
        return target.lexically_normal();
    }

    UsersRepository::UsersRepository(path_t path)
            : path_(std::move(path)) {}


    static bool ensureParentDir(const fs::path& p) {
        try {
            auto dir = p.parent_path();
            if (!dir.empty() && !fs::exists(dir)) {
                return fs::create_directories(dir);
            }
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
                {"firstName", u.firstName},
                {"lastName",  u.lastName},
                {"address",   u.address},
                {"phone",     u.phone},
                {"login",     u.login},
                {"password",  u.password},   // plaintext per your model
                {"role",      roleToString(u.role)}
        };
    }


    static bool fromJson(const json& j, User& u) {
        try {
            u.firstName = j.value("firstName", "");
            u.lastName  = j.value("lastName",  "");
            u.address   = j.value("address",   "");
            u.phone     = j.value("phone",     "");
            u.login     = j.value("login", "");
            u.password  = j.value("password",  "");
            u.role      = roleFromString(j.value("role", "GUEST"));
            return true;
        } catch (...) {
            return false;
        }
    }


    bool UsersRepository::load() {
        items_.clear();

        // Ensure folder exists; if file doesn't exist, create an empty JSON array
        if (!ensureParentDir(path_)) return false;

        if (!fs::exists(path_)) {
            // Create an empty array file: []
            std::ofstream out(path_, std::ios::trunc);
            if (!out.good()) return false;
            out << "[]";
            out.close();
            return true; // nothing to load yet
        }

        std::ifstream in(path_);
        if (!in.good()) return false;

        json doc;
        try {
            in >> doc;
        } catch (...) {
            return false; // invalid JSON
        }

        return true;
    }


    bool UsersRepository::saveAll() const {
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

    std::optional<User> UsersRepository::getByLogin(const std::string& login) const {
        auto it = std::find_if(items_.begin(), items_.end(),
                               [&](const User& u){ return u.login == login; });
        if (it == items_.end()) return std::nullopt;
        return *it;
    }

    bool UsersRepository::upsert(const User& u) {
        auto it = std::find_if(items_.begin(), items_.end(),
                               [&](const User& x){ return x.login == u.login; });
        if (it == items_.end())
            items_.push_back(u);
        else
            *it = u;
        return true;
    }

    bool UsersRepository::remove(const std::string& login) {
        auto it = std::remove_if(items_.begin(), items_.end(),
                                 [&](const User& x){ return x.login == login; });
        if (it == items_.end())
            return false;
        items_.erase(it, items_.end());
        return true;
    }

    std::vector<User> UsersRepository::list() const {
        return items_;
    }

}
