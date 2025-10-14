#include "Validators.h"
#include <cctype>

namespace hms::ui {

    bool validateLogin(const hms::UserRepository& repo, const std::string& login, std::string& why){

        if(login.size()<3) {
            why="Login must be at least 3 characters.";
            return false;
        }

        if(login.find(' ')!=std::string::npos) {
            why="Login cannot contain spaces.";
            return false;
        }

        if(repo.getByLogin(login).has_value()) {
            why="This login is already taken.";
            return false;
        }

        return true;
    }

    bool validatePasswordStrength(const std::string& pw, std::string& why)
    {
        if(pw.size()<8) {
            why="Password must be at least 8 characters.";
            return false; }

        bool up=false, lo=false, di=false;

        for(char c: pw) {
            up|=std::isupper((unsigned char)c);
            lo|=std::islower((unsigned char)c);
            di|=std::isdigit((unsigned char)c);
        }

        if(!(up&&lo&&di)) {
            why="Use upper, lower, and a digit.";
            return false;
        }
        return true;
    }
}

