#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace hms {

struct Room {
    std::string   id;                 // e.g. "RM-000123" (stable key)
    std::string   hotelId{"HTL-0001"}; // associate room with a hotel
    int           number{0};          // human-facing room number
    std::string   typeId;             // foreign key -> RoomType.id
    int           sizeSqm{0};
    int           beds{1};
    std::vector<std::string> amenities;
    std::string   notes;
    bool          active{true};
};

// JSON (de)serialization
inline void to_json(nlohmann::json& j, const Room& r) {
    j = nlohmann::json{
        {"id", r.id},
        {"hotelId", r.hotelId},
        {"number", r.number},
        {"typeId", r.typeId},
        {"sizeSqm", r.sizeSqm},
        {"beds", r.beds},
        {"amenities", r.amenities},
        {"notes", r.notes},
        {"active", r.active}
    };
}
inline void from_json(const nlohmann::json& j, Room& r) {
    j.at("hotelId").get_to(r.hotelId);
    if (j.contains("id"))      j.at("id").get_to(r.id);
    if (j.contains("number"))  j.at("number").get_to(r.number);
    if (j.contains("typeId"))  j.at("typeId").get_to(r.typeId);

    if (j.contains("sizeSqm"))   j.at("sizeSqm").get_to(r.sizeSqm);
    if (j.contains("beds"))      j.at("beds").get_to(r.beds);
    if (j.contains("amenities")) j.at("amenities").get_to(r.amenities);
    if (j.contains("notes"))     j.at("notes").get_to(r.notes);
    if (j.contains("active"))    j.at("active").get_to(r.active);

    if (r.id.empty()) {
        // Legacy data might omit a stable ID; derive one from hotelId + room number.
        const int number = r.number;
        if (!r.hotelId.empty() && number > 0) {
            r.id = r.hotelId + "-" + std::to_string(number);
        }
        else {
            r.id = "ROOM-" + std::to_string(number);
        }
    }
}

} // namespace hms
