#include "Router.h"
#include "screens/WelcomeScreen.h"
#include "screens/LoginScreen.h"
#include "screens/RegisterScreen.h"
#include "screens/DashboardGuest.h"
#include "screens/DashboardAdmin.h"
#include "core/ConsoleIO.h"
#include <iostream>
#include <stdexcept>

namespace hms::ui {

    // Helpers to persist data once at exit (or after major ops if you prefer)
    static void SaveAll(AppContext& ctx) {
        try {
            ctx.svc.users->saveAll();
            ctx.svc.rooms->saveAll();
            ctx.svc.hotels->saveAll();
            ctx.svc.bookings->saveAll();
        }
        catch (const std::exception& ex) {
            ConsoleIO::println(std::string("[WARN] Failed to save data: ") + ex.what());
        }
    }

    void Run(AppContext& ctx) {
        // Load data once, with basic error handling
        try {
            if (!ctx.svc.users->load()  ||
                !ctx.svc.rooms->load()  ||
                !ctx.svc.hotels->load() ||
                !ctx.svc.bookings->load()) {
                throw std::runtime_error("Repository load failed");
            }
        }
        catch (const std::exception& ex) {
            ConsoleIO::println(std::string("[ERROR] Failed to load data: ") + ex.what());
            ConsoleIO::println("Press Enter to exit...");
            ConsoleIO::waitKey();
            return;
        }

        while (ctx.running) {
            try {
                if (!ctx.currentUser) {
                    // ------- Welcome flow -------
                    const int choice = WelcomeScreen(); // 1=Login, 2=Register, other=Exit
                    if (choice == 1) {
                        if (auto u = LoginScreen(*ctx.svc.users)) {
                            ctx.currentUser = *u;
                        }
                    }
                    else if (choice == 2) {
                        // Allow admin bootstrap if no admins exist yet
                        if (auto u = RegisterScreen(*ctx.svc.users, /*allowAdminBootstrap=*/true)) {
                            ctx.currentUser = *u;
                        }
                    }
                    else {
                        // Exit from welcome
                        ctx.running = false;
                    }
                    continue;
                }

                // ------- Authenticated flow -------
                const auto role = ctx.currentUser->role;

                // Convention: dashboards return true if user wants to LOG OUT, false to EXIT app,
                // and keep returning to stay in dashboard.
                // If your dashboards are void, make them return an optional bool as shown below (see section 3).
                bool shouldContinue = true;
                while (ctx.running && ctx.currentUser && shouldContinue) {
                    bool logoutRequested = false;
                    if (role == hms::Role::ADMIN) {
                        logoutRequested = DashboardAdmin(ctx);
                    }
                    else if (role == hms::Role::MANAGER) {
                        logoutRequested = DashboardManager(ctx);
                    }
                    else {
                        logoutRequested = DashboardGuest(ctx);
                    }

                    if (logoutRequested) {
                        ctx.currentUser.reset();  // back to Welcome
                        SaveAll(ctx);             // persist session data on logout
                        break;
                    }

                    // If dashboard wants to exit app entirely, set ctx.running=false inside the screen.
                    shouldContinue = ctx.running && static_cast<bool>(ctx.currentUser);
                }

            }
            catch (const std::exception& ex) {
                ConsoleIO::println(std::string("[ERROR] ") + ex.what());
                ConsoleIO::println("Returning to Welcome...");
                ctx.currentUser.reset(); // drop to Welcome on any unexpected error
                ConsoleIO::waitKey();
            }
            catch (...) {
                ConsoleIO::println("[ERROR] Unknown error");
                ConsoleIO::println("Returning to Welcome...");
                ctx.currentUser.reset();
                ConsoleIO::waitKey();
            }
        }

        // Persist on final exit
        SaveAll(ctx);
    }
}
