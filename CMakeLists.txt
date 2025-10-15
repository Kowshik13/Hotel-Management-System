cmake_minimum_required(VERSION 3.20)
project(Hotel_Management_System LANGUAGES CXX)

add_compile_definitions(HMS_DATA_DIR="${CMAKE_SOURCE_DIR}/data")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if (MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(nlohmann_json CONFIG QUIET)
if (nlohmann_json_FOUND)
  message(STATUS "Using nlohmann_json from package")
else()
  message(STATUS "nlohmann_json package not found; assuming header-only include is available")
endif()

add_executable(Hotel_Management_System
  legacy/LegacyMain.cpp
  src/models/Role.h
  src/models/User.h
  src/models/Hotel.h
  src/models/RoomType.h
  src/models/Room.h
  src/models/BookingType.h
  src/models/BookingStatus.h
  src/models/BookingItem.h
  src/models/MenuItem.h
  src/models/RestaurantCategory.h
  src/models/RestaurantOrderLine.h
  src/storage/UserRepository.h
  src/storage/UserRepository.cpp
  src/Test.cpp
  src/storage/HotelRepository.h
  src/storage/HotelRepository.cpp
  src/storage/RoomsRepository.h
  src/storage/RoomsRepository.cpp
  src/models/Restaurant.h
  src/models/RoomStayItem.h
  src/models/Booking.h
  src/storage/BookingRepository.h
  src/storage/BookingRepository.cpp
  src/ui/AppContext.h
  src/ui/core/ConsoleIO.h
  src/ui/core/ConsoleIO.cpp
  src/ui/core/Validators.h
  src/ui/core/Validators.cpp
  src/ui/screens/LoginScreen.h
  src/ui/screens/LoginScreen.cpp
  src/ui/screens/RegisterScreen.h
  src/ui/screens/RegisterScreen.cpp
  src/ui/screens/DashboardGuest.h
  src/ui/screens/DashboardGuest.cpp
  src/ui/screens/DashboardAdmin.h
  src/ui/screens/DashboardAdmin.cpp
  src/ui/screens/WelcomeScreen.h
  src/ui/screens/WelcomeScreen.cpp
  src/ui/Router.h
  src/ui/Router.cpp
  src/Main.cpp
)

# 👇 NEW: make <nlohmann/json.hpp> resolve to external/nlohmann/json.hpp
target_include_directories(Hotel_Management_System PRIVATE
  ${CMAKE_SOURCE_DIR}/external
)

if (nlohmann_json_FOUND)
  target_link_libraries(Hotel_Management_System PRIVATE nlohmann_json::nlohmann_json)
endif()
