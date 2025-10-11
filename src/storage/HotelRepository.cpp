#include "HotelRepository.h"
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


    HotelRepository::path_t HotelRepository::defaultPath() {
        fs::path p = fs::current_path() / ".." / "src" / "data" / "hotels.json";
        return p.lexically_normal();
    }


    HotelRepository::HotelRepository(path_t path) : path_(std::move(path)) {}


    static json toJson(const Hotel& h) {
        return json{
                {"id", h.id},
                {"name", h.name},
                {"stars", h.stars},
                {"address", h.address}
        };
    }


    static bool fromJson(const json& j, Hotel& h) {
        try {
            h.id      = j.at("id").get<std::string>();
            h.name    = j.value("name", "");
            h.stars   = j.value("stars", 0u);
            h.address = j.value("address", "");
            return true;
        } catch (...) { return false; }
    }


    bool HotelRepository::load() {
        items_.clear();
        if (!ensureParentDir(path_)) return false;

        if (!fs::exists(path_)) {
            std::ofstream out(path_, std::ios::trunc);
            if (!out.good()) return false;
            out << "[]";          // empty array
            return true;
        }

        std::ifstream in(path_);
        if (!in.good()) return false;

        json doc;
        try { in >> doc; } catch (...) { return false; }
        if (!doc.is_array()) return false;

        for (const auto& j : doc) {
            Hotel h{};
            if (fromJson(j, h)) items_.push_back(std::move(h));
        }
        return true;
    }


    bool HotelRepository::saveAll() const {
        if (!ensureParentDir(path_)) return false;
        const fs::path tmp = path_.string() + ".tmp";

        json arr = json::array();
        for (const auto& h : items_) arr.push_back(toJson(h));

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


    std::optional<Hotel> HotelRepository::get(const std::string& id) const {
        auto it = std::find_if(items_.begin(), items_.end(),
                               [&](const Hotel& h){ return h.id == id; });
        if (it == items_.end()) return std::nullopt;
        return *it;
    }


    bool HotelRepository::upsert(const Hotel& h) {
        auto it = std::find_if(items_.begin(), items_.end(),
                               [&](const Hotel& x){ return x.id == h.id; });
        if (it == items_.end()) items_.push_back(h);
        else                    *it = h;
        return true;
    }


    bool HotelRepository::remove(const std::string& id) {
        auto it = std::remove_if(items_.begin(), items_.end(),
                                 [&](const Hotel& x){ return x.id == id; });
        if (it == items_.end()) return false;
        items_.erase(it, items_.end());
        return true;
    }


    std::vector<Hotel> HotelRepository::list() const { return items_; }

}
