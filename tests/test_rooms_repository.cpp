#include <gtest/gtest.h>
#include "../src/storage/RoomsRepository.h"
#include "_test_support.h"

using namespace hms;
using test_support::TempDir;

static Room makeRoom(std::string hotelId, int number, bool active=true, std::string id="") {
    Room r{};
    r.hotelId = std::move(hotelId);
    r.number = number;
    r.active = active;
    r.id = std::move(id);
    return r;
}

TEST(RoomsRepository, LoadBootstrapAndRoundTrip) {
TempDir tmp;
RoomsRepository repo{tmp.join("data/rooms.json")};
ASSERT_TRUE(repo.load());
EXPECT_TRUE(repo.list().empty());

ASSERT_TRUE(repo.upsert(makeRoom("H1",101,true)));   // id will be normalized "H1-101"
ASSERT_TRUE(repo.upsert(makeRoom("H1",102,false)));
ASSERT_TRUE(repo.upsert(makeRoom("H2",201,true)));
ASSERT_TRUE(repo.saveAll());

RoomsRepository again{repo.resolvedPath()};
ASSERT_TRUE(again.load());

auto r = again.get("H1", 101);
ASSERT_TRUE(r.has_value());
EXPECT_EQ(r->id, "H1-101");           // normalization check
EXPECT_EQ(again.listByHotel("H1").size(), 2u);
EXPECT_EQ(again.countActiveByHotel("H1"), 1);
}

TEST(RoomsRepository, UpsertReplaceAndRemove) {
TempDir tmp;
RoomsRepository repo{tmp.join("rooms.json")};
ASSERT_TRUE(repo.load());

ASSERT_TRUE(repo.upsert(makeRoom("H1",101,true)));
ASSERT_TRUE(repo.upsert(makeRoom("H1",101,false)));
auto r = repo.get("H1",101);
ASSERT_TRUE(r.has_value());
EXPECT_FALSE(r->active);

EXPECT_TRUE(repo.remove("H1",101));
EXPECT_FALSE(repo.get("H1",101).has_value());
EXPECT_FALSE(repo.remove("H1",101));
}

TEST(RoomsRepository, LoadRejectsInvalidTopLevelJson) {
TempDir tmp;
auto path = tmp.join("rooms.json");
test_support::write_text(path, R"({"no":"array"})");
RoomsRepository repo{path};
EXPECT_FALSE(repo.load());
}
