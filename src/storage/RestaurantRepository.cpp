#include "RestaurantRepository.h"

#include <algorithm>
#include <fstream>
#include <filesystem>
#include <system_error>
#include <cstdint>

#include <nlohmann/json.hpp>

using nlohmann::json;
namespace fs = std::filesystem;

namespace {

fs::path baseDataDir() {
#ifdef HMS_DATA_DIR
    return fs::path{HMS_DATA_DIR};
#else
    return (fs::current_path() / "data").lexically_normal();
#endif
}

bool ensureParentDir(const fs::path& p) {
    try {
        auto dir = p.parent_path();
        if (!dir.empty() && !fs::exists(dir)) {
            return fs::create_directories(dir);
        }
        return true;
    }
    catch (...) {
        return false;
    }
}

json toJson(const hms::MenuItem& item) {
    return json{
        {"id", item.id},
        {"restaurantId", item.restaurantId},
        {"name", item.name},
        {"category", item.category},
        {"priceCents", item.priceCents},
        {"active", item.active}
    };
}

bool fromJson(const json& j, hms::MenuItem& item) {
    try {
        item.id          = j.value("id", "");
        item.restaurantId= j.value("restaurantId", "");
        item.name        = j.value("name", "");
        item.category    = j.value("category", "");
        item.priceCents  = j.value("priceCents", static_cast<std::int64_t>(0));
        item.active      = j.value("active", true);
        return true;
    }
    catch (...) {
        return false;
    }
}

json toJson(const hms::Restaurant& r) {
    json menu = json::array();
    for (const auto& item : r.menu) {
        menu.push_back(toJson(item));
    }
    return json{
        {"id", r.id},
        {"hotelId", r.hotelId},
        {"name", r.name},
        {"cuisine", r.cuisine},
        {"openHours", r.openHours},
        {"active", r.active},
        {"menu", menu}
    };
}

bool fromJson(const json& j, hms::Restaurant& r) {
    try {
        r.id        = j.value("id", "");
        r.hotelId   = j.value("hotelId", "");
        r.name      = j.value("name", "");
        r.cuisine   = j.value("cuisine", "");
        r.openHours = j.value("openHours", "");
        r.active    = j.value("active", true);
        r.menu.clear();
        if (j.contains("menu") && j.at("menu").is_array()) {
            for (const auto& entry : j.at("menu")) {
                hms::MenuItem item{};
                if (fromJson(entry, item)) {
                    if (item.restaurantId.empty()) item.restaurantId = r.id;
                    r.menu.push_back(std::move(item));
                }
            }
        }
        return true;
    }
    catch (...) {
        return false;
    }
}

} // namespace

namespace hms {

RestaurantRepository::path_t RestaurantRepository::defaultPath() {
    return (baseDataDir() / "restaurants.json").lexically_normal();
}

RestaurantRepository::RestaurantRepository(path_t path)
    : path_(std::move(path)) {}

bool RestaurantRepository::load() {
    items_.clear();
    if (!ensureParentDir(path_)) return false;

    if (!fs::exists(path_)) {
        std::ofstream out(path_, std::ios::trunc);
        if (!out.good()) return false;
        out << "[]\n";
        return true;
    }

    std::ifstream in(path_);
    if (!in.good()) return false;

    json doc;
    try { in >> doc; }
    catch (...) { return false; }

    if (doc.is_null()) return true;
    if (!doc.is_array()) return false;

    for (const auto& entry : doc) {
        Restaurant r{};
        if (fromJson(entry, r)) {
            for (auto& item : r.menu) {
                if (item.restaurantId.empty()) item.restaurantId = r.id;
            }
            items_.push_back(std::move(r));
        }
    }
    return true;
}

bool RestaurantRepository::saveAll() const {
    if (!ensureParentDir(path_)) return false;

    const fs::path tmp = path_.string() + ".tmp";
    json arr = json::array();
    for (const auto& r : items_) arr.push_back(toJson(r));

    {
        std::ofstream out(tmp, std::ios::trunc);
        if (!out.good()) return false;
        out << arr.dump(2) << '\n';
        out.flush();
        if (!out.good()) return false;
    }

    std::error_code ec;
    fs::rename(tmp, path_, ec);
    if (ec) {
        fs::remove(tmp);
        return false;
    }
    return true;
}

std::optional<Restaurant> RestaurantRepository::get(const std::string& id) const {
    auto it = std::find_if(items_.begin(), items_.end(),
                           [&](const Restaurant& r){ return r.id == id; });
    if (it == items_.end()) return std::nullopt;
    return *it;
}

std::vector<Restaurant> RestaurantRepository::list() const {
    return items_;
}

std::vector<Restaurant> RestaurantRepository::listByHotel(const std::string& hotelId) const {
    std::vector<Restaurant> out;
    for (const auto& r : items_) {
        if (r.hotelId == hotelId) {
            out.push_back(r);
        }
    }
    return out;
}

bool RestaurantRepository::upsert(const Restaurant& restaurant) {
    auto it = std::find_if(items_.begin(), items_.end(),
                           [&](const Restaurant& r){ return r.id == restaurant.id; });
    if (it == items_.end()) {
        items_.push_back(restaurant);
    }
    else {
        *it = restaurant;
    }
    return true;
}

bool RestaurantRepository::remove(const std::string& id) {
    auto it = std::remove_if(items_.begin(), items_.end(),
                             [&](const Restaurant& r){ return r.id == id; });
    if (it == items_.end()) return false;
    items_.erase(it, items_.end());
    return true;
}

} // namespace hms
