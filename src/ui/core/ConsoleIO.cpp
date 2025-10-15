#include "ConsoleIO.h"
#include <iostream>
#include <algorithm>
#include <cctype>

#if defined(_WIN32)
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

namespace hms::ui {

    std::string trim(std::string s) {
        auto notsp= [](int ch){
            return !std::isspace(ch);
        };
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), notsp));
        s.erase(std::find_if(s.rbegin(), s.rend(), notsp).base(), s.end());
        return s;
    }

    std::string readLine(const std::string& prompt, bool allowEmpty) {
        for(;;) {
            std::cout<<prompt;
            std::string s; std::getline(std::cin, s);
            if (std::cin.fail()) {
                std::cin.clear();
                continue;
            }
            s=trim(s);
            if (!allowEmpty && s.empty()) {
                std::cout<<"Please enter a value.\n"; continue;
            }
            return s;
        }
    }
    std::string readPassword(const std::string& prompt) {
        std::cout<<prompt;
        std::string pw;
#if defined(_WIN32)
        for(;;){
        int ch=_getch();
        if(ch=='\r'||ch=='\n'){ std::cout<<"\n"; break; }
        if(ch==3){ std::cout<<"\n"; break; } // Ctrl+C
        if(ch==8||ch==127){ if(!pw.empty()) pw.pop_back(); continue; }
        if(ch>=32 && ch<=126) pw.push_back(static_cast<char>(ch));
    }
#else
        termios oldt{}; tcgetattr(STDIN_FILENO,&oldt);
        termios newt=oldt; newt.c_lflag &= ~ECHO;
        tcsetattr(STDIN_FILENO,TCSANOW,&newt);
        std::getline(std::cin,pw);
        tcsetattr(STDIN_FILENO,TCSANOW,&oldt);
        std::cout<<"\n";
#endif
        return pw;
    }

    void banner(const std::string& title){
        std::cout<<"\n==============================\n";
        std::cout<<"  "<<title<<"\n";
        std::cout<<"==============================\n";
    }

    void pause() {
        std::cout<<"Press Enter to continue...";
        std::string _; std::getline(std::cin,_);
    }
}

void ConsoleIO::println(const std::string& s) { std::cout << s << '\n'; }
void ConsoleIO::print(const std::string& s) { std::cout << s; std::cout.flush(); }

void ConsoleIO::waitKey() {
    std::cout << "(Press Enter)"; std::cout.flush();
    std::string _; std::getline(std::cin, _);
}

void ConsoleIO::clear() {
#ifdef _WIN32
    std::system("cls");
#else
    std::system("clear");
#endif
}

// Optional helpers (safe, line-based)
int ConsoleIO::readInt() {
    for (;;) {
        std::string s = readLine("", /*allowEmpty=*/false);
        try {
            size_t idx = 0;
            int v = std::stoi(s, &idx, 10);
            if (idx == s.size()) return v;
        }
        catch (...) {}
        std::cout << "Please enter a valid integer: ";
    }
}

long long ConsoleIO::readInt64() {
    for (;;) {
        std::string s = readLine("", /*allowEmpty=*/false);
        try {
            size_t idx = 0;
            long long v = std::stoll(s, &idx, 10);
            if (idx == s.size()) return v;
        }
        catch (...) {}
        std::cout << "Please enter a valid 64-bit integer: ";
    }
}

} // namespace hms::ui