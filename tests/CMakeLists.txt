cmake_minimum_required(VERSION 3.20)

file(GLOB HMS_STORAGE_SOURCES  "${CMAKE_SOURCE_DIR}/src/storage/*.cpp")
file(GLOB HMS_SECURITY_SOURCES "${CMAKE_SOURCE_DIR}/src/security/*.cpp")
file(GLOB HMS_MODEL_SOURCES    "${CMAKE_SOURCE_DIR}/src/models/*.cpp")
file(GLOB TEST_SOURCES         "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp")

add_executable(hms_repo_tests
        ${TEST_SOURCES}
        ${HMS_STORAGE_SOURCES}
        ${HMS_SECURITY_SOURCES}
        ${HMS_MODEL_SOURCES}
        test_sample_data.cpp
)

target_include_directories(hms_repo_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/external
)

target_compile_features(hms_repo_tests PRIVATE cxx_std_20)

target_link_libraries(hms_repo_tests PRIVATE
        GTest::gtest_main
        GTest::gmock
)

if (TARGET nlohmann_json::nlohmann_json)
    target_link_libraries(hms_repo_tests PRIVATE nlohmann_json::nlohmann_json)
endif()

include(GoogleTest)

# On Windows, the Visual Studio generator relies on PowerShell (pwsh.exe) when
# running the test discovery helper that backs gtest_discover_tests.  Some
# environments – for example fresh VS Code + CMake setups – do not ship with
# PowerShell in PATH which makes the build emit `'pwsh.exe' is not recognized`
# and the configuration fails.  Registering the test executable explicitly with
# add_test avoids that requirement while still allowing `ctest` to execute the
# suite.
add_test(NAME hms_repo_tests COMMAND hms_repo_tests)
