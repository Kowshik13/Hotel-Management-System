#include "Router.h"
#include "screens/WelcomeScreen.h"
#include "screens/LoginScreen.h"
#include "screens/RegisterScreen.h"
#include "screens/DashboardGuest.h"
#include "screens/DashboardAdmin.h"
#include "core/ConsoleIO.h"
#include <iostream>

namespace hms::ui {
    void Run(AppContext& ctx) {
        // Load data once
        ctx.svc.users->load();
        ctx.svc.rooms->load();
        ctx.svc.hotels->load();
        ctx.svc.bookings->load();

        while(ctx.running) {
            if (!ctx.currentUser) {
                int choice = WelcomeScreen();
                if (choice==1) {
                    auto u = LoginScreen(*ctx.svc.users);
                    if(u) {
                        ctx.currentUser = *u;
                    }
                } else if (choice==2) {
                    // Allow admin bootstrap if no admins exist yet
                    auto u = RegisterScreen(*ctx.svc.users, /*allowAdminBootstrap=*/true);
                    if(u) {
                        ctx.currentUser = *u;
                    }
                } else {
                    ctx.running=false;
                }
            } else {
                auto role = ctx.currentUser->role;
                if (role == hms::Role::ADMIN)
                    DashboardAdmin(ctx);
                else
                    DashboardGuest(ctx);
            }
        }
    }
}
