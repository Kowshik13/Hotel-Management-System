#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "../models/Restaurant.h"

namespace hms {
    class RestaurantRepository {
    public:
        using path_t = std::filesystem::path;

        explicit RestaurantRepository(path_t path = defaultPath());

        bool load();
        bool saveAll() const;

        std::optional<Restaurant> get(const std::string& id) const;
        std::vector<Restaurant> list() const;
        std::vector<Restaurant> listByHotel(const std::string& hotelId) const;

        bool upsert(const Restaurant& restaurant);
        bool remove(const std::string& id);

        static path_t defaultPath();
        const path_t& resolvedPath() const { return path_; }

    private:
        path_t path_;
        std::vector<Restaurant> items_;
    };
}
