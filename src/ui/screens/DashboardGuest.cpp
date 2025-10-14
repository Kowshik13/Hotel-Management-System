#include "../AppContext.h"
#include "DashboardGuest.h"
#include "../core/ConsoleIO.h"
#include <iostream>


namespace hms::ui {
    void DashboardGuest(hms::AppContext& ctx) {
        for(;;) {
            banner("Guest Dashboard");
            std::cout<<"1) Book a room (todo)\n";
            std::cout<<"2) Reserve restaurant (todo)\n";
            std::cout<<"3) Profile (todo)\n";
            std::cout<<"4) Logout\n";
            auto c = readLine("Select: ");
            if(c=="4") {
                ctx.currentUser.reset();
                return;
            }
            std::cout<<"Coming soon.\n"; pause();
        }
    }
}