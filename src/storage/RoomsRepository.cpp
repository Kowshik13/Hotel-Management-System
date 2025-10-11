// src/storage/RoomsRepository.cpp
#include "RoomsRepository.h"
#include <algorithm>
#include <fstream>
#include <cstdio>
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

    RoomsRepository::path_t RoomsRepository::defaultPath() {
        fs::path p = fs::current_path() / ".." / "src" / "data" / "rooms.json";
        return p.lexically_normal();
    }

    RoomsRepository::RoomsRepository(path_t path) : path_(std::move(path)) {}

    static json toJson(const Room& r) {
        return json{
                {"hotelId",   r.hotelId},
                {"number",    r.number},
                {"typeId",    r.typeId},
                {"sizeSqm",   r.sizeSqm},
                {"beds",      r.beds},
                {"amenities", r.amenities},
                {"notes",     r.notes},
                {"active",    r.active}
        };
    }

    static bool fromJson(const json& j, Room& r) {
        try {
            r.hotelId = j.at("hotelId").get<std::string>();
            r.number  = j.at("number").get<int>();
            r.typeId  = j.value("typeId", "");
            r.sizeSqm = j.value("sizeSqm", 0);
            r.beds    = j.value("beds", 1);
            r.amenities = j.value("amenities", std::vector<std::string>{});
            r.notes   = j.value("notes", "");
            r.active  = j.value("active", true);
            return true;
        } catch (...) { return false; }
    }

    bool RoomsRepository::load() {
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

        for (const auto& j : doc) {
            Room r{};
            if (fromJson(j, r)) items_.push_back(std::move(r));
        }
        return true;
    }

    bool RoomsRepository::saveAll() const {
        if (!ensureParentDir(path_)) return false;
        const fs::path tmp = path_.string() + ".tmp";

        json arr = json::array();
        for (const auto& r : items_) arr.push_back(toJson(r));

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

    std::optional<Room> RoomsRepository::get(const std::string& hotelId, int number) const {
        auto it = std::find_if(items_.begin(), items_.end(),
                               [&](const Room& r){ return r.hotelId == hotelId && r.number == number; });
        if (it == items_.end()) return std::nullopt;
        return *it;
    }

    bool RoomsRepository::upsert(const Room& r) {
        auto it = std::find_if(items_.begin(), items_.end(),
                               [&](const Room& x){ return x.hotelId == r.hotelId && x.number == r.number; });
        if (it == items_.end()) items_.push_back(r);
        else                    *it = r;
        return true;
    }

    bool RoomsRepository::remove(const std::string& hotelId, int number) {
        auto it = std::remove_if(items_.begin(), items_.end(),
                                 [&](const Room& x){ return x.hotelId == hotelId && x.number == number; });
        if (it == items_.end()) return false;
        items_.erase(it, items_.end());
        return true;
    }

    std::vector<Room> RoomsRepository::list() const {
        return items_;
    }

    std::vector<Room> RoomsRepository::listByHotel(const std::string& hotelId) const {
        std::vector<Room> out;
        for (const auto& r : items_) if (r.hotelId == hotelId) out.push_back(r);
        return out;
    }

    int RoomsRepository::countActiveByHotel(const std::string& hotelId) const {
        int n = 0;
        for (const auto& r : items_) if (r.hotelId == hotelId && r.active) ++n;
        return n;
    }

}

