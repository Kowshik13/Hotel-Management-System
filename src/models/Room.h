#pragma once
#include <string>
#include <vector>

namespace hms {
    struct Room {
        int room_number;           // unique (101, 102, ...)
        std::string typeId;             // references RoomType.id (e.g., "RT-DELUXE")
        int roomSize{0};         // actual size; Admin-editable
        int numberOfBeds{1};            // number of sleeping places
        std::vector<std::string> amenities; // ["AC","TV","Sea View"]
        std::string notes;      // any additional information
        bool activeRoom{true};       // block booking if false (maintenance)
    };
}
