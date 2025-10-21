#include <gtest/gtest.h>
#include "../src/storage/BookingRepository.h"
#include "_test_support.h"

using namespace hms;
using test_support::TempDir;
namespace fs = std::filesystem;

TEST(BookingRepository, LoadCreatesEmptyFileIfMissing) {
TempDir tmp;
auto path = tmp.join("data/bookings.json"); // parent doesn't exist yet
BookingRepository repo{path};
ASSERT_TRUE(repo.load());
EXPECT_TRUE(fs::exists(path));
EXPECT_TRUE(repo.list().empty());
}

TEST(BookingRepository, LoadFailsOnInvalidJson) {
TempDir tmp;
auto path = tmp.join("data/bookings.json");
test_support::write_text(path, "{ not json ]");
BookingRepository repo{path};
EXPECT_FALSE(repo.load()); // parse error -> false
}

TEST(BookingRepository, LoadFailsOnNonArrayJson) {
TempDir tmp;
auto path = tmp.join("data/bookings.json");
test_support::write_text(path, R"({"oops":"object, not array"})");
BookingRepository repo{path};
EXPECT_FALSE(repo.load()); // requires array
}

TEST(BookingRepository, SaveAllWritesArrayAndIsAtomicish) {
TempDir tmp;
auto path = tmp.join("data/bookings.json");
BookingRepository repo{path};
ASSERT_TRUE(repo.load()); // ensures parent dir + file exists
ASSERT_TRUE(repo.saveAll()); // writes "[]\n"
EXPECT_TRUE(fs::exists(path));
EXPECT_FALSE(fs::exists(fs::path{path.string()+".tmp"}));
}

