#include <gtest/gtest.h>
#include "../src/storage/UserRepository.h"
#include "_test_support.h"

using namespace hms;
using test_support::TempDir;

static User makeUser(std::string id, std::string login, Role role=Role::GUEST, bool active=true) {
    User u{};
    u.userId = std::move(id);
    u.login = std::move(login);
    u.role = role;
    u.active = active;
    u.firstName = "F";
    u.lastName = "L";
    u.address = "Addr";
    u.phone = "000";
    u.passwordHash = "HASH";
    return u;
}

TEST(UserRepository, LoadBootstrapAndRoundTrip) {
TempDir tmp;
UserRepository repo{tmp.join("data/users.json")};
ASSERT_TRUE(repo.load());
EXPECT_TRUE(repo.list().empty());

ASSERT_TRUE(repo.upsert(makeUser("U1","alice")));
ASSERT_TRUE(repo.upsert(makeUser("U2","bob", Role::ADMIN)));
ASSERT_TRUE(repo.saveAll());

UserRepository again{repo.resolvedPath()};
ASSERT_TRUE(again.load());
auto a = again.getByLogin("alice");
auto b = again.getById("U2");
ASSERT_TRUE(a.has_value());
ASSERT_TRUE(b.has_value());
EXPECT_EQ(b->role, Role::ADMIN);
}

TEST(UserRepository, UniqueLoginEnforcedOnUpsert) {
TempDir tmp;
UserRepository repo{tmp.join("users.json")};
ASSERT_TRUE(repo.load());
ASSERT_TRUE(repo.upsert(makeUser("U1","dup")));
EXPECT_FALSE(repo.upsert(makeUser("U2","dup"))); //taken by other
EXPECT_EQ(repo.list().size(), 1u);

// Changing U1 to a new unique login is fine
auto u1 = makeUser("U1","unique");
EXPECT_TRUE(repo.upsert(u1));
}

TEST(UserRepository, RemoveByIdAndLogin) {
TempDir tmp;
UserRepository repo{tmp.join("users.json")};
ASSERT_TRUE(repo.load());
ASSERT_TRUE(repo.upsert(makeUser("U1","a")));
ASSERT_TRUE(repo.upsert(makeUser("U2","b")));

EXPECT_TRUE(repo.removeById("U1"));
EXPECT_FALSE(repo.getById("U1").has_value());
EXPECT_TRUE(repo.removeByLogin("b"));
EXPECT_FALSE(repo.getByLogin("b").has_value());

EXPECT_FALSE(repo.removeById("nope"));
EXPECT_FALSE(repo.removeByLogin("nope"));
}

TEST(UserRepository, LoadRejectsInvalidTopLevelJson) {
TempDir tmp;
auto path = tmp.join("users.json");
test_support::write_text(path, R"({"array?":false})");
UserRepository repo{path};
EXPECT_FALSE(repo.load());
}

TEST(UserRepository, LoadLegacyPasswordFieldHashesAndClears) {
TempDir tmp;
auto path = tmp.join("users.json");
// Contains "password" (legacy) but not "passwordHash".
test_support::write_text(path, R"([
    {"userId":"U1","login":"legacy","password":"cleartext","firstName":"F","lastName":"L","address":"","phone":"","role":"GUEST","active":true}
  ])");
UserRepository repo{path};
ASSERT_TRUE(repo.load()); // fromJson() migrates password -> passwordHash

auto u = repo.getByLogin("legacy");
ASSERT_TRUE(u.has_value());
EXPECT_TRUE(!u->passwordHash.empty()); // now hashed
EXPECT_TRUE(u->password.empty());  // cleared
}
