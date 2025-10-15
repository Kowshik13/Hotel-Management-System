#include "RoomsRepository.h"

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

} // namespace

namespace hms {

RoomsRepository::path_t RoomsRepository::defaultPath() {
    return (baseDataDir() / "rooms.json").lexically_normal();
}

RoomsRepository::RoomsRepository(path_t path)
    : path_(std::move(path)) {}

bool RoomsRepository::load() {
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

    try {
        items_ = doc.get<std::vector<Room>>();
    }
    catch (...) {
        return false;
    }
    return true;
}

bool RoomsRepository::saveAll() const {
    if (!ensureParentDir(path_)) return false;

    const fs::path tmp = path_.string() + ".tmp";
    json arr = json::array();
    for (const auto& r : items_) arr.push_back(r);

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

std::optional<Room> RoomsRepository::get(const std::string& hotelId, int number) const {
    auto it = std::find_if(items_.begin(), items_.end(),
                           [&](const Room& r){ return r.hotelId == hotelId && r.number == number; });
    if (it == items_.end()) return std::nullopt;
    return *it;
}

bool RoomsRepository::upsert(const Room& r) {
    Room normalized = r;
    if (normalized.id.empty() && !normalized.hotelId.empty() && normalized.number > 0) {
        normalized.id = normalized.hotelId + "-" + std::to_string(normalized.number);
    }

    auto it = std::find_if(items_.begin(), items_.end(),
                           [&](const Room& x){ return x.hotelId == normalized.hotelId && x.number == normalized.number; });
    if (it == items_.end()) items_.push_back(std::move(normalized));
    else                    *it = std::move(normalized);
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

} // namespace hms
