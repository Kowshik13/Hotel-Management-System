// src/models/BookingStatus.h
#pragma once
#include <nlohmann/json.hpp>

namespace hms {

enum class BookingStatus { ACTIVE, CHECKED_OUT, CANCELLED };

NLOHMANN_JSON_SERIALIZE_ENUM(BookingStatus, {
    {BookingStatus::ACTIVE, "ACTIVE"},
    {BookingStatus::CHECKED_OUT, "CHECKED_OUT"},
    {BookingStatus::CANCELLED, "CANCELLED"}
})

} // namespace hms
