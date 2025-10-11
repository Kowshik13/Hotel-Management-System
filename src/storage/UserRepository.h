#pragma once
#include <string>
#include <vector>
#include <optional>
#include <filesystem>
#include "../models/User.h"

namespace hms {

    class UsersRepository {
    public:
        using path_t = std::filesystem::path;

        explicit UsersRepository(path_t path = defaultUsersPath());

        bool load();
        bool saveAll() const;

        std::optional<User> getByLogin(const std::string& login) const;
        bool upsert(const User& u);
        bool remove(const std::string& login);
        std::vector<User> list() const;

        // Expose the resolved absolute path (handy for debugging)
        const path_t& resolvedPath() const { return path_; }

        // CWD/../src/data/users.json  (then lexically normalized)
        static path_t defaultUsersPath();

    private:
        path_t path_;
        std::vector<User> items_;
    };

}
