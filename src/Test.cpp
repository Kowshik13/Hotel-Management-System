#include <iostream>
#include <filesystem>
#include "storage/UserRepository.h"
#include "models/User.h"
#include "models/Role.h"
#include "storage/HotelRepository.h"
#include "storage/RoomsRepository.h"

namespace fs = std::filesystem;

int main() {
    hms::UsersRepository repo;

    if (!repo.load()) { std::cerr << "Load failed\n"; return 1; }
    std::cout << "Users file: " << repo.resolvedPath() << "\n";

    hms::User u{};
    u.firstName = "John";
    u.lastName  = "Doe";
    u.address   = "MG Road";
    u.phone     = "9876543210";
    u.login     = "john";
    u.password  = "secret";
    u.role      = hms::Role::GUEST;
    repo.upsert(u);

    hms::User a{};
    a.firstName = "Jane";
    a.lastName  = "Admin";
    a.address   = "Baker Street";
    a.phone     = "1234567890";
    a.login     = "admin";
    a.password  = "admin123";
    a.role      = hms::Role::ADMIN;
    repo.upsert(a);

    if (!repo.saveAll()) { std::cerr << "Save failed\n"; return 1; }

    repo.load();
    for (const auto& it : repo.list()) {
        std::cout << it.login << " (" << (it.role == hms::Role::ADMIN ? "ADMIN" : it.role == hms::Role::GUEST ? "USER" : "GUEST") << ")\n";
    }

    hms::HotelRepository hotels;
    hms::RoomsRepository rooms;
    hotels.load();
    rooms.load();

    hms::Hotel h{};
    h.id = "HTL-0001";
    h.name = "Resort beach";
    h.stars = 4;
    h.address = "Antalya, Istanbul";
    hotels.upsert(h);
    hotels.saveAll();

    hms::Room r{};
    r.hotelId="HTL-0001"; r.number=101; r.typeId="DELUXE";
    r.sizeSqm=24; r.beds=1; r.amenities={"AC","TV"}; r.active=true;
    rooms.upsert(r);
    rooms.saveAll();

    std::cout << "Active rooms in " << h.id << ": " << rooms.countActiveByHotel("HTL-0001") << "\n";

    return 0;
}
