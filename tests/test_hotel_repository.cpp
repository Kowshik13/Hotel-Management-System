#include <gtest/gtest.h>
#include "../src/storage/HotelRepository.h"
#include "_test_support.h"

using namespace hms;
using test_support::TempDir;
namespace fs = std::filesystem;

static Hotel makeHotel(std::string id, std::string name, uint8_t stars, std::string addr) {
    Hotel h{};
    h.id = std::move(id);
    h.name = std::move(name);
    h.stars = stars;
    h.address = std::move(addr);
    return h;
}

TEST(HotelRepository, LoadBootstrapAndRoundTrip) {
TempDir tmp;
auto path = tmp.join("data/hotels.json");
HotelRepository repo{path};
ASSERT_TRUE(repo.load());
EXPECT_TRUE(repo.list().empty());

// upsert + save
ASSERT_TRUE(repo.upsert(makeHotel("H1","Alpha",3,"Main St")));
ASSERT_TRUE(repo.upsert(makeHotel("H2","Beta", 5,"Ocean Ave")));
ASSERT_TRUE(repo.saveAll());

// reload in fresh instance
HotelRepository again{path};
ASSERT_TRUE(again.load());
auto h1 = again.get("H1");
auto h2 = again.get("H2");
ASSERT_TRUE(h1.has_value());
ASSERT_TRUE(h2.has_value());
EXPECT_EQ(h1->name, "Alpha");
EXPECT_EQ(h2->stars, 5);
}

TEST(HotelRepository, UpsertReplaceAndRemove) {
TempDir tmp;
HotelRepository repo{tmp.join("hotels.json")};
ASSERT_TRUE(repo.load());

ASSERT_TRUE(repo.upsert(makeHotel("H1","Alpha",3,"A")));
ASSERT_TRUE(repo.upsert(makeHotel("H1","Alpha+,renamed",4,"A2"))); // replace same id
auto h = repo.get("H1");
ASSERT_TRUE(h.has_value());
EXPECT_EQ(h->name, "Alpha+,renamed");
EXPECT_EQ(h->stars, 4);

EXPECT_TRUE(repo.remove("H1"));
EXPECT_FALSE(repo.get("H1").has_value());
EXPECT_FALSE(repo.remove("H1")); // already gone
}

TEST(HotelRepository, LoadRejectsInvalidTopLevelJson) {
TempDir tmp;
auto path = tmp.join("hotels.json");
test_support::write_text(path, R"({"not":"array"})");
HotelRepository repo{path};
EXPECT_FALSE(repo.load());
}

TEST(HotelRepository, LoadSkipsBadElementsButSucceeds) {
TempDir tmp;
auto path = tmp.join("hotels.json");
// second item has wrong type for "stars" -> fromJson returns false and is skipped
test_support::write_text(path, R"([
    {"id":"H1","name":"Good","stars":3,"address":"A"},
    {"id":"H_BAD","name":"Bad","stars":"five","address":"B"}
  ])");
HotelRepository repo{path};
EXPECT_TRUE(repo.load());
EXPECT_TRUE(repo.get("H1").has_value());
EXPECT_FALSE(repo.get("H_BAD").has_value());
EXPECT_EQ(repo.list().size(), 1u);
}

