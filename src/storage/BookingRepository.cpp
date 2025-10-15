#include "BookingRepository.h"

#include <algorithm>
#include <fstream>
#include <filesystem>
#include <iterator>
#include <system_error>
#include <utility>

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

BookingRepository::path_t BookingRepository::defaultPath() {
    return (baseDataDir() / "bookings.json").lexically_normal();
}

BookingRepository::BookingRepository(path_t path)
        : path_(std::move(path)) {}

bool BookingRepository::load() {
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
        items_ = doc.get<std::vector<Booking>>();
    }
    catch (...) {
        return false;
    }
    return true;
}

bool BookingRepository::saveAll() const {
    if (!ensureParentDir(path_)) return false;

    const fs::path tmp = path_.string() + ".tmp";
    json arr = json::array();
    for (const auto& b : items_) arr.push_back(b);

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

std::optional<Booking> BookingRepository::get(const std::string& bookingId) const {
    auto it = std::find_if(items_.begin(), items_.end(),
                           [&](const Booking& b){ return b.bookingId == bookingId; });
    if (it == items_.end()) return std::nullopt;
    return *it;
}

bool BookingRepository::upsert(const Booking& b) {
    if (b.bookingId.empty()) return false;
    auto it = std::find_if(items_.begin(), items_.end(),
                           [&](const Booking& x){ return x.bookingId == b.bookingId; });
    if (it == items_.end()) items_.push_back(b);
    else                    *it = b;
    return true;
}

bool BookingRepository::remove(const std::string& bookingId) {
    auto it = std::remove_if(items_.begin(), items_.end(),
                             [&](const Booking& x){ return x.bookingId == bookingId; });
    if (it == items_.end()) return false;
    items_.erase(it, items_.end());
    return true;
}

std::vector<Booking> BookingRepository::list() const {
    return items_;
}

std::vector<Booking> BookingRepository::listActive() const {
    std::vector<Booking> out;
    std::copy_if(items_.begin(), items_.end(), std::back_inserter(out),
                 [](const Booking& b){ return b.status == BookingStatus::ACTIVE; });
    return out;
}

std::vector<Booking> BookingRepository::listByHotel(const std::string& hotelId) const {
    std::vector<Booking> out;
    std::copy_if(items_.begin(), items_.end(), std::back_inserter(out),
                 [&](const Booking& b){ return b.hotelId == hotelId; });
    return out;
}

} // namespace hms
