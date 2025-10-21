#include "_test_support.h"

#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>

#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

namespace {
    nlohmann::json loadCatalogue() {
        fs::path root = fs::path{__FILE__}.parent_path();
        root = root.parent_path();
        const fs::path file = root / "src" / "data" / "catalogue.json";
        std::ifstream in(file);
        if (!in.is_open()) {
            ADD_FAILURE() << "Could not open sample catalogue at " << file;
            return {};
        }

        nlohmann::json doc;
        in >> doc;
        return doc;
    }
}

TEST(SampleData, CatalogueStructureIsConsistent) {
    const auto doc = loadCatalogue();
    ASSERT_TRUE(doc.is_object());

    ASSERT_TRUE(doc.contains("hotels"));
    ASSERT_TRUE(doc.contains("rooms"));
    ASSERT_TRUE(doc.contains("restaurants"));

    const auto& hotels = doc.at("hotels");
    const auto& rooms = doc.at("rooms");
    const auto& restaurants = doc.at("restaurants");

    ASSERT_TRUE(hotels.is_array());
    ASSERT_TRUE(rooms.is_array());
    ASSERT_TRUE(restaurants.is_array());

    ASSERT_GE(hotels.size(), 3) << "Expect at least three demo hotels";
    ASSERT_GE(rooms.size(), hotels.size()) << "Should have at least as many rooms as hotels";
    ASSERT_GE(restaurants.size(), 3) << "Expect a restaurant for each demo hotel";

    std::set<std::string> hotelIds;
    for (const auto& hotel : hotels) {
        ASSERT_TRUE(hotel.is_object());
        ASSERT_TRUE(hotel.contains("id"));
        ASSERT_TRUE(hotel.contains("name"));
        ASSERT_TRUE(hotel.contains("address"));
        ASSERT_TRUE(hotel.contains("stars"));

        const auto id = hotel.at("id").get<std::string>();
        ASSERT_FALSE(id.empty());
        hotelIds.insert(id);

        const auto stars = hotel.at("stars").get<int>();
        EXPECT_GE(stars, 1);
        EXPECT_LE(stars, 5);
    }

    SCOPED_TRACE("Hotels available: " + std::to_string(hotelIds.size()));

    for (const auto& room : rooms) {
        ASSERT_TRUE(room.is_object());
        const auto hotelId = room.at("hotelId").get<std::string>();
        ASSERT_TRUE(hotelIds.count(hotelId) > 0) << "Room references unknown hotel " << hotelId;
        EXPECT_TRUE(room.contains("number"));
        EXPECT_TRUE(room.contains("typeId"));
        EXPECT_TRUE(room.contains("beds"));
        EXPECT_TRUE(room.contains("amenities"));

        if (room.contains("amenities")) {
            const auto& amenities = room.at("amenities");
            ASSERT_TRUE(amenities.is_array());
            for (const auto& amenity : amenities) {
                EXPECT_TRUE(amenity.is_string());
            }
        }
    }

    for (const auto& restaurant : restaurants) {
        ASSERT_TRUE(restaurant.is_object());
        const auto hotelId = restaurant.at("hotelId").get<std::string>();
        ASSERT_TRUE(hotelIds.count(hotelId) > 0) << "Restaurant references unknown hotel " << hotelId;
        ASSERT_TRUE(restaurant.contains("menu"));

        const auto& menu = restaurant.at("menu");
        ASSERT_TRUE(menu.is_array());
        ASSERT_FALSE(menu.empty());

        for (const auto& item : menu) {
            ASSERT_TRUE(item.is_object());
            EXPECT_TRUE(item.contains("id"));
            EXPECT_TRUE(item.contains("name"));
            EXPECT_TRUE(item.contains("category"));
            EXPECT_TRUE(item.contains("priceCents"));

            if (item.contains("priceCents")) {
                const auto price = item.at("priceCents").get<int>();
                EXPECT_GT(price, 0) << "Menu price must be positive";
            }
        }
    }
}

TEST(SampleData, RestaurantsCoverEveryHotel) {
    const auto doc = loadCatalogue();
    ASSERT_TRUE(doc.is_object());

    const auto& hotels = doc.at("hotels");
    const auto& restaurants = doc.at("restaurants");

    std::set<std::string> hotelsWithoutDining;
    for (const auto& hotel : hotels) {
        hotelsWithoutDining.insert(hotel.at("id").get<std::string>());
    }

    for (const auto& restaurant : restaurants) {
        hotelsWithoutDining.erase(restaurant.at("hotelId").get<std::string>());
    }

    if (!hotelsWithoutDining.empty()) {
        std::ostringstream oss;
        oss << "Missing restaurants for hotel ids: ";
        bool first = true;
        for (const auto& id : hotelsWithoutDining) {
            if (!first) oss << ", ";
            oss << id;
            first = false;
        }
        ADD_FAILURE() << oss.str();
    }

    EXPECT_TRUE(hotelsWithoutDining.empty());
}
