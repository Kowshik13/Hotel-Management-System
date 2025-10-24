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
gtest_discover_tests(hms_repo_tests)
