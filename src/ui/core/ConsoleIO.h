#pragma once
#include <string>

namespace hms::ui {
    std::string trim(std::string s);
    std::string readLine(const std::string& prompt, bool allowEmpty=false);
    std::string readPassword(const std::string& prompt);
    void        banner(const std::string& title);
    void        pause();
}

struct ConsoleIO {
    static void println(const std::string& s);
    static void print(const std::string& s);
    static void waitKey();
    static void clear();

    static int         readInt();    // optional, handy for menus
    static long long   readInt64();  // optional, for timestamps etc.
};
}