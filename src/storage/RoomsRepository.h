#pragma once
#include <string>
#include <vector>
#include <optional>
#include <filesystem>
#include "../models/Room.h"

namespace hms {
    class RoomsRepository {
    public:
        using path_t = std::filesystem::path;
        explicit RoomsRepository(path_t path = defaultPath());

        bool load();
        bool saveAll() const;


        std::optional<Room> get(const std::string& hotelId, int number) const;
        bool upsert(const Room& r);
        bool remove(const std::string& hotelId, int number);

        std::vector<Room> list() const;
        std::vector<Room> listByHotel(const std::string& hotelId) const;
        int countActiveByHotel(const std::string& hotelId) const;

        static path_t defaultPath();   // e.g., CWD/../src/data/rooms.json
        const path_t& resolvedPath() const { return path_; }

    private:
        path_t path_;
        std::vector<Room> items_;
    };
}