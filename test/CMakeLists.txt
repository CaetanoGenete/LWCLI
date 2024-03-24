include(GoogleTest)

find_package(GTest CONFIG REQUIRED)

function(add_lwcli_test test_name test_source)
    add_executable(${test_name} ${test_source})
    target_link_libraries(${test_name} PRIVATE ${PROJECT_NAME} GTest::gtest GTest::gtest_main)
    gtest_discover_tests(${test_name})
endfunction()

add_lwcli_test(cast_tests cast_tests.cpp)
add_lwcli_test(assert_tests assert_tests.cpp)
add_lwcli_test(integration integration.cpp)