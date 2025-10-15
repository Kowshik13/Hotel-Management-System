#include "DashboardAdmin.h"
#include "../core/ConsoleIO.h"
#include <iostream>

namespace hms::ui {

bool DashboardAdmin(hms::AppContext& ctx) {
    for (;;) {
        banner("Admin Dashboard");
        std::cout << "1) Manage hotels (todo)\n";
        std::cout << "2) Manage rooms (todo)\n";
        std::cout << "3) View bookings (todo)\n";
        std::cout << "4) Logout\n";
        std::cout << "0) Exit application\n";
        const auto choice = readLine("Select: ");

        if (choice == "4") {
            return true; // logout
        }
        if (choice == "0") {
            ctx.running = false;
            return false; // exit app entirely
        }

        std::cout << "Coming soon.\n";
        pause();
    }
}

} // namespace hms::ui
