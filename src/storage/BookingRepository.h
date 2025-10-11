#pragma once
#include <string>
#include <vector>
#include <optional>
#include <filesystem>
#include "../models/Booking.h"

namespace hms {

    class BookingRepository {
    public:
        using path_t = std::filesystem::path;

        explicit BookingRepository(path_t path = defaultPath());

        // Disk I/O
        bool load();          // read JSON -> memory (creates file if missing)
        bool saveAll() const; // memory -> JSON (atomic via temp+rename)

        // CRUD (in-memory; caller decides when to saveAll)
        std::optional<Booking> get(const std::string& bookingId) const;
        bool upsert(const Booking& b);
        bool remove(const std::string& bookingId);

        // Convenience queries
        std::vector<Booking> list() const;
        std::vector<Booking> listActive() const;
        std::vector<Booking> listByHotel(const std::string& hotelId) const;

        // Paths
        static path_t defaultPath();                 // CWD/../src/data/bookings.json (normalized)
        const path_t& resolvedPath() const { return path_; }

    private:
        path_t path_;
        std::vector<Booking> items_;
    };

} // namespace hms
