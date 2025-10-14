#include "WelcomeScreen.h"
#include "../core/ConsoleIO.h"
#include <iostream>
namespace hms::ui {
    int WelcomeScreen() {
        banner("Welcome to HMS");
        std::cout<<"1) Login\n";
        std::cout<<"2) Register\n";
        std::cout<<"3) Exit\n";
        auto c = readLine("Select: ");
        if (c=="1") return 1;
        if(c=="2") return 2;
        return 3;
    }
}
