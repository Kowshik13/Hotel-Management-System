#pragma once
#include <string>

namespace hms {
    enum class BookingStatus { ACTIVE, CHECKED_OUT, CANCELLED };

    inline std::string to_string(BookingStatus s) {
        switch (s) {
            case BookingStatus::ACTIVE:      return "ACTIVE";
            case BookingStatus::CHECKED_OUT: return "CHECKED_OUT";
            case BookingStatus::CANCELLED:   return "CANCELLED";
        }
        return "ACTIVE";
    }
}

