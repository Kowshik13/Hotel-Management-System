#pragma once
#include <string>

namespace hms::ui {
    std::string trim(std::string s);
    std::string readLine(const std::string& prompt, bool allowEmpty=false);
    std::string readPassword(const std::string& prompt);
    void        banner(const std::string& title);
    void        pause();
}