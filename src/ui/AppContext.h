#pragma once
#include <optional>
#include "../models/User.h"
#include "../storage/UserRepository.h"
#include "../storage/RoomsRepository.h"
#include "../storage/HotelRepository.h"
#include "../storage/BookingRepository.h"
#include "../storage/RestaurantRepository.h"

namespace hms {

    struct Services {
        UserRepository*       users{};
        RoomsRepository*      rooms{};
        HotelRepository*      hotels{};
        BookingRepository*    bookings{};
        RestaurantRepository* restaurants{};
        // Add AuthService later (hashes, lockouts, etc.)
    };

    struct AppContext {
        Services          svc;
        std::optional<User> currentUser; // set after successful login
        bool               running{true};
    };

}
