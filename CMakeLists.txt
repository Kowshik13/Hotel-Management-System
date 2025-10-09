cmake_minimum_required(VERSION 4.0)
project(Hotel_Management_System)

set(CMAKE_CXX_STANDARD 20)

add_executable(Hotel_Management_System legacy/LegacyMain.cpp
        legacy/LegacyMain.cpp
        src/models/Role.h
        src/models/Guest.h
        src/models/Hotel.h
        src/models/RoomType.h
        src/models/Room.h
        src/models/BookingType.h
        src/models/BookingStatus.h
        src/models/BookingItem.h
        src/models/MenuItem.h
        src/models/RestaurantCategory.h
        src/models/RestaurantOrder.h
)
