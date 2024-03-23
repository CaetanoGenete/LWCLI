include(GoogleTest)

find_package(GTest CONFIG REQUIRED)

add_executable(cast_tests cast_tests.cpp)
target_link_libraries(cast_tests PRIVATE ${PROJECT_NAME} GTest::gtest GTest::gtest_main)

gtest_discover_tests(cast_tests)