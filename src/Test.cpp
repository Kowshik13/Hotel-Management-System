//#include <iostream>
//#include <filesystem>
//#include <variant>
//#include <cassert>
//
//// 🔁 Adjust this include to your actual header name:
//#include "storage/UserRepository.h"   // or "storage/UserRepository.h"
//#include "storage/HotelRepository.h"
//#include "storage/RoomsRepository.h"
//#include "storage/BookingRepository.h"
//
//#include "models/User.h"
//#include "models/Role.h"
//#include "models/Booking.h"            // must bring in Booking, RoomStayItem, RestaurantOrderLine, BookingStatus
//
//namespace fs = std::filesystem;
//
//static void printPass(const std::string& msg) { std::cout << "✅ " << msg << "\n"; }
//static void printFail(const std::string& msg) { std::cout << "❌ " << msg << "\n"; }
//
//int main() {
//    int fail = 0;
//    auto expect = [&](bool cond, const std::string& msg){
//        if (cond) printPass(msg);
//        else { printFail(msg); ++fail; }
//    };
//
//    std::cout << "CWD = " << fs::current_path() << "\n\n";
//
//    // -------------------------
//    // 1) USERS
//    // -------------------------
//    hms::UserRepository usersRepo;      // default path: CWD/../src/data/users.json
//    fs::remove(usersRepo.resolvedPath()); // start clean
//
//    expect(usersRepo.load(), "Users load() creates empty file if missing");
//    std::cout << "Users file: " << usersRepo.resolvedPath() << "\n";
//
//    // Create 2 users (NOTE: userId is primary key)
//    hms::User admin{};
//    admin.userId = "USR-0001";
//    admin.firstName = "Jane"; admin.lastName = "Admin";
//    admin.address = "Baker Street"; admin.phone = "1234567890";
//    admin.login = "admin"; admin.password = "admin123"; admin.role = hms::Role::ADMIN;
//    expect(usersRepo.upsert(admin), "Users upsert(admin)");
//
//    hms::User guest{};
//    guest.userId = "USR-0002";
//    guest.firstName = "John"; guest.lastName = "Doe";
//    guest.address = "MG Road"; guest.phone = "9876543210";
//    guest.login = "john"; guest.password = "secret"; guest.role = hms::Role::GUEST;
//    expect(usersRepo.upsert(guest), "Users upsert(guest)");
//
//    expect(usersRepo.saveAll(), "Users saveAll()");
//    expect(usersRepo.load(), "Users reload() after save");
//
//    {
//        auto list = usersRepo.list();
//        expect(list.size() >= 2, "Users list() has at least 2 entries");
//        auto byLogin = usersRepo.getByLogin("admin");
//        expect(byLogin && byLogin->userId == "USR-0001", "Users getByLogin(admin) -> USR-0001");
//        auto byId = usersRepo.getById("USR-0002");
//        expect(byId && byId->login == "john", "Users getById(USR-0002) -> john");
//    }
//
//    // -------------------------
//    // 2) HOTELS & ROOMS
//    // -------------------------
//    hms::HotelRepository hotelsRepo;   fs::remove(hotelsRepo.resolvedPath());
//    hms::RoomsRepository roomsRepo;    fs::remove(roomsRepo.resolvedPath());
//
//    expect(hotelsRepo.load(), "Hotels load() creates empty file");
//    expect(roomsRepo.load(),  "Rooms load() creates empty file");
//    std::cout << "Hotels file: " << hotelsRepo.resolvedPath() << "\n";
//    std::cout << "Rooms  file: " << roomsRepo.resolvedPath()  << "\n";
//
//    hms::Hotel h{};
//    h.id = "HTL-0001"; h.name = "Resort Beach"; h.stars = 4; h.address = "Antalya, Türkiye";
//    expect(hotelsRepo.upsert(h), "Hotels upsert(HTL-0001)");
//    expect(hotelsRepo.saveAll(), "Hotels saveAll()");
//    expect(hotelsRepo.load(),    "Hotels reload()");
//
//    hms::Room r1{};
//    r1.hotelId = "HTL-0001"; r1.number = 101; r1.typeId = "DELUXE";
//    r1.sizeSqm = 24; r1.beds = 1; r1.amenities = {"AC","TV"}; r1.active = true;
//    expect(roomsRepo.upsert(r1), "Rooms upsert(room 101)");
//
//    hms::Room r2{};
//    r2.hotelId = "HTL-0001"; r2.number = 102; r2.typeId = "STANDARD";
//    r2.sizeSqm = 20; r2.beds = 2; r2.amenities = {"AC"}; r2.active = true;
//    expect(roomsRepo.upsert(r2), "Rooms upsert(room 102)");
//
//    expect(roomsRepo.saveAll(), "Rooms saveAll()");
//    expect(roomsRepo.load(),    "Rooms reload()");
//    expect(roomsRepo.countActiveByHotel("HTL-0001") == 2, "Rooms countActiveByHotel(HTL-0001) == 2");
//
//    {
//        auto allRooms = roomsRepo.listByHotel("HTL-0001");
//        expect(allRooms.size() >= 2, "Rooms listByHotel returns rooms");
//        auto r = roomsRepo.get("HTL-0001", 101);
//        expect(r && r->typeId == "DELUXE", "Rooms get(HTL-0001,101) -> DELUXE");
//    }
//
//    // -------------------------
//    // 3) BOOKINGS (RoomStayItem + RestaurantOrderLine)
//    // -------------------------
//    hms::BookingRepository bookingsRepo; fs::remove(bookingsRepo.resolvedPath());
//    expect(bookingsRepo.load(), "Bookings load() creates empty file");
//    std::cout << "Bookings file: " << bookingsRepo.resolvedPath() << "\n";
//
//    hms::Booking b{};
//    b.bookingId = "BKG-000001";
//    b.hotelId = "HTL-0001";
//    b.status  = hms::BookingStatus::ACTIVE;
//    b.createdAt = b.updatedAt = 1728800000; // demo epoch
//    b.primaryGuestId = "GST-000001";
//
//    // Room stay
//    hms::RoomStayItem stay{};
//    stay.hotelId = "HTL-0001";
//    stay.roomNumber = 101;
//    stay.nights = 2;
//    stay.nightlyRateLocked = 15000;
//    hms::User occ1{"GST-000001","Alice","Brown","MG Road","98765",""};
//    stay.occupants.push_back(occ1);
//    b.items.push_back(stay);
//
//    // Restaurant line
//    hms::RestaurantOrderLine line{};
//    line.lineId = "ROL-0001";
//    line.restaurantId = "RST-0001";
//    line.category = "Breakfast";
//    line.menuItemId = "bf-omelette";
//    line.nameSnapshot = "Masala Omelette";
//    line.unitPriceSnapshot = 200;
//    line.qty = 2;
//    line.billedRoomNumber = 101;
//    line.takenByUsername = "kitchen1";
//    line.orderedByGuestId = "GST-000001";
//    line.createdAt = 1728800100;
//    b.items.push_back(line);
//
//    expect(bookingsRepo.upsert(b), "Bookings upsert(BKG-000001)");
//    expect(bookingsRepo.saveAll(), "Bookings saveAll()");
//    expect(bookingsRepo.load(),    "Bookings reload()");
//
//    // Verify round-trip
//    auto got = bookingsRepo.get("BKG-000001");
//    expect(!!got, "Bookings get(BKG-000001) returns value");
//    if (got) {
//        expect(got->hotelId == "HTL-0001", "Booking.hotelId round-trips");
//        expect(got->items.size() == 2, "Booking.items size == 2");
//
//        // Item[0] should be RoomStayItem
//        const auto& it0 = got->items[0];
//        expect(std::holds_alternative<hms::RoomStayItem>(it0), "Item[0] is RoomStayItem");
//        if (std::holds_alternative<hms::RoomStayItem>(it0)) {
//            const auto& rs = std::get<hms::RoomStayItem>(it0);
//            expect(rs.roomNumber == 101 && rs.nights == 2, "RoomStayItem fields round-trip");
//            expect(!rs.occupants.empty() && rs.occupants[0].userId == "GST-000001", "RoomStayItem.occupants[0].guestId");
//        }
//
//        // Item[1] should be RestaurantOrderLine
//        const auto& it1 = got->items[1];
//        expect(std::holds_alternative<hms::RestaurantOrderLine>(it1), "Item[1] is RestaurantOrderLine");
//        if (std::holds_alternative<hms::RestaurantOrderLine>(it1)) {
//            const auto& rl = std::get<hms::RestaurantOrderLine>(it1);
//            expect(rl.menuItemId == "bf-omelette" && rl.qty == 2, "RestaurantOrderLine fields round-trip");
//            expect(rl.orderedByGuestId == "GST-000001", "RestaurantOrderLine.orderedByGuestId");
//        }
//    }
//
//    // Lists
//    auto active = bookingsRepo.listActive();
//    expect(!active.empty(), "listActive() has at least one item");
//    auto byHotel = bookingsRepo.listByHotel("HTL-0001");
//    expect(!byHotel.empty(), "listByHotel(HTL-0001) has items");
//
//    expect(bookingsRepo.upsert(b), "Bookings upsert(BKG-000001)");
//    expect(bookingsRepo.saveAll(), "Bookings saveAll()");
//
//
//
//    // -------------------------
//    // Summary
//    // -------------------------
//    std::cout << "\n---- SUMMARY ----\n";
//    if (fail == 0) {
//        std::cout << "All repository tests passed.\n";
//    } else {
//        std::cout << fail << " test(s) failed.\n";
//    }
//    return fail == 0 ? 0 : 1;
//}
