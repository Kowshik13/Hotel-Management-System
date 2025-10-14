#include "DashboardAdmin.h"
#include "../core/ConsoleIO.h"
#include <iostream>

namespace hms::ui {
    void DashboardAdmin(hms::AppContext& ctx) {
        for(;;) {
            banner("Admin Dashboard");
            std::cout<<"1) Manage hotels (todo)\n";
            std::cout<<"2) Manage rooms (todo)\n";
            std::cout<<"3) View bookings (todo)\n";
            std::cout<<"4) Logout\n";
            auto c = readLine("Select: ");
            if(c=="4"){ ctx.currentUser.reset(); return; }
            std::cout<<"Coming soon.\n"; pause();
        }
    }
}