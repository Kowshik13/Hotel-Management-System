#include "BookingRepository.h"

#include <algorithm>
#include <fstream>
#include <cstdio>
#include <utility>
#include <filesystem>
#include "../external/nlohmann/json.hpp"

using nlohmann::json;
namespace fs = std::filesystem;

namespace hms {


    static bool ensureParentDir(const fs::path& p) {
        try {
            auto d = p.parent_path();
            if (!d.empty() && !fs::exists(d)) return fs::create_directories(d);
            return true;
        } catch (...) { return false; }
    }

    BookingRepository::path_t BookingRepository::defaultPath() {
        fs::path p = fs::current_path() / ".." / "src" / "data" / "bookings.json";
        return p.lexically_normal();
    }

    BookingRepository::BookingRepository(path_t path)
            : path_(std::move(path)) {}


    static std::string statusToString(BookingStatus s) {
        switch (s) {
            case BookingStatus::ACTIVE:      return "ACTIVE";
            case BookingStatus::CHECKED_OUT: return "CHECKED_OUT";
            case BookingStatus::CANCELLED:   return "CANCELLED";
        }
        return "ACTIVE";
    }

    static BookingStatus statusFromString(const std::string& s) {
        if (s == "CHECKED_OUT") return BookingStatus::CHECKED_OUT;
        if (s == "CANCELLED")   return BookingStatus::CANCELLED;
        return BookingStatus::ACTIVE;
    }

    static json toJson(const User& g) {
        return json{
                {"userId",   g.userId},
                {"firstName", g.firstName},
                {"lastName",  g.lastName},
                {"address",   g.address},
                {"phone",     g.phone},
        };
    }
    static bool fromJson(const json& j, User& g) {
        try {
            g.userId    = j.value("userId", "");
            g.firstName = j.value("firstName", "");
            g.lastName  = j.value("lastName",  "");
            g.address   = j.value("address",   "");
            g.phone     = j.value("phone",     "");
            return true;
        } catch (...) { return false; }
    }

// RoomStayItem
    static json toJson(const RoomStayItem& r) {
        json occ = json::array();
        for (const auto& g : r.occupants) occ.push_back(toJson(g));
        return json{
                {"kind",              "RoomStayItem"},
                {"hotelId",           r.hotelId},
                {"roomNumber",        r.roomNumber},
                {"nights",            r.nights},
                {"nightlyRateLocked", r.nightlyRateLocked},
                {"occupants",         occ}
        };
    }
    static bool fromJson(const json& j, RoomStayItem& r) {
        try {
            r.hotelId           = j.at("hotelId").get<std::string>();
            r.roomNumber        = j.at("roomNumber").get<int>();
            r.nights            = j.value("nights", 1);
            r.nightlyRateLocked = j.value("nightlyRateLocked", (std::int64_t)0);

            r.occupants.clear();
            if (j.contains("occupants") && j["occupants"].is_array()) {
                for (const auto& jg : j["occupants"]) {
                    User g{};
                    if (fromJson(jg, g)) r.occupants.push_back(std::move(g));
                }
            }
            return true;
        } catch (...) { return false; }
    }


    static json toJson(const RestaurantOrderLine& o) {
        return json{
                {"kind",              "RestaurantOrderLine"},
                {"lineId",            o.lineId},
                {"restaurantId",      o.restaurantId},
                {"category",          o.category},
                {"menuItemId",        o.menuItemId},
                {"nameSnapshot",      o.nameSnapshot},
                {"unitPriceSnapshot", o.unitPriceSnapshot},
                {"qty",               o.qty},
                {"billedRoomNumber",  o.billedRoomNumber},
                {"takenByUsername",   o.takenByUsername},
                {"orderedByGuestId",  o.orderedByGuestId},
                {"createdAt",         o.createdAt}
        };
    }

    static bool fromJson(const json& j, RestaurantOrderLine& o) {
        try {
            o.lineId            = j.at("lineId").get<std::string>();
            o.restaurantId      = j.value("restaurantId", "");
            o.category          = j.value("category", "");
            o.menuItemId        = j.value("menuItemId", "");
            o.nameSnapshot      = j.value("nameSnapshot", "");
            o.unitPriceSnapshot = j.value("unitPriceSnapshot", (std::int64_t)0);
            o.qty               = j.value("qty", 1);
            o.billedRoomNumber  = j.value("billedRoomNumber", 0);
            o.takenByUsername   = j.value("takenByUsername", "");
            o.orderedByGuestId  = j.value("orderedByGuestId", "");
            o.createdAt         = j.value("createdAt", (std::int64_t)0);
            return true;
        } catch (...) { return false; }
    }


    static json toJson(const BookingItem& it) {
        if (std::holds_alternative<RoomStayItem>(it))        return toJson(std::get<RoomStayItem>(it));
        if (std::holds_alternative<RestaurantOrderLine>(it)) return toJson(std::get<RestaurantOrderLine>(it));
        return json{}; // should not happen
    }


    static bool fromJson(const json& j, BookingItem& it) {
        const std::string kind = j.value("kind", "");
        if (kind == "RoomStayItem") {
            RoomStayItem r{};
            if (!fromJson(j, r)) return false;
            it = std::move(r);
            return true;
        }
        if (kind == "RestaurantOrderLine") {
            RestaurantOrderLine o{};
            if (!fromJson(j, o)) return false;
            it = std::move(o);
            return true;
        }
        return false; // unknown kind
    }


    static json toJson(const Booking& b) {
        json items = json::array();
        for (const auto& it : b.items) items.push_back(toJson(it));
        return json{
                {"bookingId",            b.bookingId},
                {"hotelId",              b.hotelId},
                {"status",               statusToString(b.status)},
                {"createdAt",            b.createdAt},
                {"updatedAt",            b.updatedAt},
                {"primaryGuestId",       b.primaryGuestId},
                {"items",                items}
        };
    }
    static bool fromJson(const json& j, Booking& b) {
        try {
            b.bookingId            = j.at("bookingId").get<std::string>();
            b.hotelId              = j.value("hotelId", "");
            b.status               = statusFromString(j.value("status", "ACTIVE"));
            b.createdAt            = j.value("createdAt", (std::int64_t)0);
            b.updatedAt            = j.value("updatedAt", (std::int64_t)0);
            b.primaryGuestId       = j.value("primaryGuestId", "");

            b.items.clear();
            if (j.contains("items") && j["items"].is_array()) {
                for (const auto& ji : j["items"]) {
                    BookingItem it;
                    if (fromJson(ji, it)) b.items.push_back(std::move(it));
                }
            }
            return true;
        } catch (...) { return false; }
    }


    bool BookingRepository::load() {
        items_.clear();
        if (!ensureParentDir(path_)) return false;

        if (!fs::exists(path_)) {
            std::ofstream out(path_, std::ios::trunc);
            if (!out.good()) return false;
            out << "[]";
            return true;
        }

        std::ifstream in(path_);
        if (!in.good()) return false;

        json doc;
        try { in >> doc; } catch (...) { return false; }
        if (!doc.is_array()) return false;

        for (const auto& jb : doc) {
            Booking b{};
            if (fromJson(jb, b)) items_.push_back(std::move(b));
        }
        return true;
    }

    bool BookingRepository::saveAll() const {
        if (!ensureParentDir(path_)) return false;

        const fs::path dir = path_.parent_path();
        const std::string base = path_.filename().string();
        const fs::path tmp = dir / (base + ".tmp");

        // Build JSON array
        json arr = json::array();
        for (const auto& b : items_) arr.push_back(toJson(b));

        // Write temp file
        {
            std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
            if (!out.good()) return false;

            // If you prefer exceptions on failure:
            // out.exceptions(std::ofstream::failbit | std::ofstream::badbit);

            out << arr.dump(2);
            out.flush();
            if (!out.good()) {
                out.close();
                std::error_code ec;
                fs::remove(tmp, ec); // best-effort cleanup
                return false;
            }
            out.close();
        }

        // Try atomic rename first (works on POSIX; on Windows fails if target exists)
        std::error_code ec;
        fs::rename(tmp, path_, ec);
        if (!ec) return true;

        // Windows-friendly fallback: remove existing then rename
        fs::remove(path_, ec); // ignore error if doesn't exist
        ec.clear();
        fs::rename(tmp, path_, ec);
        if (!ec) return true;

        // Last resort: copy+overwrite then remove tmp
        ec.clear();
        fs::copy_file(tmp, path_, fs::copy_options::overwrite_existing, ec);
        std::error_code ec2;
        fs::remove(tmp, ec2);
        return !ec; // success if copy succeeded
    }


    std::optional<Booking> BookingRepository::get(const std::string& id) const {
        auto it = std::find_if(items_.begin(), items_.end(),
                               [&](const Booking& b){ return b.bookingId == id; });
        if (it == items_.end()) return std::nullopt;
        return *it; // copy
    }

    bool BookingRepository::upsert(const Booking& b) {
        auto it = std::find_if(items_.begin(), items_.end(),
                               [&](const Booking& x){ return x.bookingId == b.bookingId; });
        if (it == items_.end()) items_.push_back(b);
        else                    *it = b;
        return true;
    }

    bool BookingRepository::remove(const std::string& id) {
        auto it = std::remove_if(items_.begin(), items_.end(),
                                 [&](const Booking& x){ return x.bookingId == id; });
        if (it == items_.end()) return false;
        items_.erase(it, items_.end());
        return true;
    }

    std::vector<Booking> BookingRepository::list() const {
        return items_;
    }

    std::vector<Booking> BookingRepository::listActive() const {
        std::vector<Booking> out;
        for (const auto& b : items_)
            if (b.status == BookingStatus::ACTIVE) out.push_back(b);
        return out;
    }

    std::vector<Booking> BookingRepository::listByHotel(const std::string& hotelId) const {
        std::vector<Booking> out;
        for (const auto& b : items_)
            if (b.hotelId == hotelId) out.push_back(b);
        return out;
    }

}
