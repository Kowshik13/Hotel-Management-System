#pragma once
#include <string>
#include <cstdint>

namespace hms {
    struct RoomType {
        std::string  id;          // "RT-DELUXE" (stable key)
        std::string name;        // "Deluxe"
        std::int64_t nightlyRate; // money in smallest unit (e.g., paise/cents)
        bool active{true};
    };
}
