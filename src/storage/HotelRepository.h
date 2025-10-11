#pragma once
#include <string>
#include <vector>
#include <optional>
#include <filesystem>
#include "../models/Hotel.h"

namespace hms {
    class HotelRepository {
    public:
        using path_t = std::filesystem::path;
        explicit HotelRepository(path_t path = defaultPath());

        bool load();
        bool saveAll() const;

        std::optional<Hotel> get(const std::string& id) const;
        bool upsert(const Hotel& h);     // add or replace by id
        bool remove(const std::string& id);
        std::vector<Hotel> list() const;

        static path_t defaultPath();     // e.g., CWD/../src/data/hotels.json (normalized)
        const path_t& resolvedPath() const { return path_; }

    private:
        path_t path_;
        std::vector<Hotel> items_;
    };
}
