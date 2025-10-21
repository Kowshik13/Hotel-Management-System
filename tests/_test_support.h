#pragma once
#include <filesystem>
#include <fstream>
#include <string>
#include <cstdlib>
#include <stdexcept>
#include <system_error>

namespace test_support {
    namespace fs = std::filesystem;

    struct TempDir {
        fs::path dir;
        TempDir() {
            auto base = fs::temp_directory_path();
            for (int i = 0; i < 1000; ++i) {
                auto candidate = base / ("hms-tests-" + std::to_string(std::rand()));
                std::error_code ec;
                if (fs::create_directory(candidate, ec)) { dir = candidate; return; }
            }
            throw std::runtime_error("TempDir: failed to create");
        }
        ~TempDir() {
            std::error_code ec;
            fs::remove_all(dir, ec);
        }
        fs::path join(const std::string& name) const { return dir / name; }
    };

    inline void write_text(const std::filesystem::path& p, const std::string& s) {
        std::error_code ec;
        auto parent = p.parent_path();
        if (!parent.empty() && !std::filesystem::exists(parent)) {
            std::filesystem::create_directories(parent, ec);
        }
        std::ofstream out(p, std::ios::trunc);
        out << s;
    }
}
