#include "DashboardGuest.h"
#include "../core/ConsoleIO.h"
#include <iostream>

namespace hms::ui {

bool DashboardGuest(hms::AppContext& ctx) {
    for (;;) {
        banner("Guest Dashboard");
        std::cout << "1) Book a room (todo)\n";
        std::cout << "2) Reserve restaurant (todo)\n";
        std::cout << "3) Profile (todo)\n";
        std::cout << "4) Logout\n";
        std::cout << "0) Exit application\n";
        const auto choice = readLine("Select: ");

        if (choice == "4") {
            return true;
        }
        if (choice == "0") {
            ctx.running = false;
            return false;
        }

        std::cout << "Coming soon.\n";
        pause();
    }
}

} // namespace hms::ui
