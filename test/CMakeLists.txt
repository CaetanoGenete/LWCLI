include(GoogleTest)

find_package(GTest CONFIG REQUIRED)

add_executable(cast_tests cast_tests.cpp)
target_link_libraries(cast_tests PRIVATE ${PROJECT_NAME} GTest::gtest GTest::gtest_main)

add_executable(integration integration.cpp)
target_link_libraries(integration PRIVATE ${PROJECT_NAME} GTest::gtest GTest::gtest_main)

gtest_discover_tests(cast_tests)
gtest_discover_tests(integration)