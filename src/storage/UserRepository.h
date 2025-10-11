#pragma once
#include <string>
#include <vector>
#include <optional>
#include <filesystem>
#include "../models/User.h"

namespace hms {

    class UserRepository {
    public:
        using path_t = std::filesystem::path;

        // Default path: CWD/../src/data/users.json (normalized)
        explicit UserRepository(path_t path = defaultUsersPath());

        // Disk I/O
        bool load();          // reads JSON -> memory (creates empty file if missing)
        bool saveAll() const; // memory -> JSON (atomic via temp+rename)

        // Queries
        std::optional<User> getById(const std::string& userId) const;
        std::optional<User> getByLogin(const std::string& login) const;
        std::vector<User>   list() const;

        bool upsert(const User& u);
        bool removeById(const std::string& userId);
        bool removeByLogin(const std::string& login);

        // Utils
        static path_t defaultUsersPath();
        const path_t& resolvedPath() const { return path_; }

    private:
        bool loginTakenByOther(const std::string& login, const std::string& thisUserId) const;

    private:
        path_t path_;
        std::vector<User> items_;
    };

} // namespace hms
