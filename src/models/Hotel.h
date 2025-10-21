#pragma once
#include <string>
#include <cstdint>

namespace hms {
    struct Hotel {
        std::string id = "HTL-0001";
        std::string name;
        std::uint8_t stars{3};
        std::string address;
        std::int64_t baseRateCents{0};
        std::string managerUserId; // optional link to the managing user

        // Derived counts are not stored here; compute from repositories:
        // roomsCount = RoomsRepository.countActive()
        // restaurantsCount = RestaurantsRepository.countActive()
    };
}
