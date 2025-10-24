#include "HotelRepository.h"

#include <algorithm>
#include <fstream>
#include <filesystem>
#include <system_error>

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

json toJson(const hms::Hotel& h) {
    return json{
        {"id", h.id},
        {"name", h.name},
        {"stars", h.stars},
        {"address", h.address}
    };
}

bool fromJson(const json& j, hms::Hotel& h) {
    try {
        h.id      = j.value("id", "");
        h.name    = j.value("name", "");
        h.stars   = static_cast<std::uint8_t>(j.value("stars", 0));
        h.address = j.value("address", "");
        return true;
    }
    catch (...) {
        return false;
    }
}

}

namespace hms {

HotelRepository::path_t HotelRepository::defaultPath() {
    return (baseDataDir() / "hotels.json").lexically_normal();
}

HotelRepository::HotelRepository(path_t path)
    : path_(std::move(path)) {}

bool HotelRepository::load() {
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

    for (const auto& j : doc) {
        Hotel h{};
        if (fromJson(j, h)) {
            items_.push_back(std::move(h));
        }
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

std::vector<Hotel> HotelRepository::list() const {
    return items_;
}

}
