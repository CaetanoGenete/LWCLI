find_package(benchmark CONFIG REQUIRED)

add_executable(string_vs_map string_vs_map.cpp)
target_link_libraries(string_vs_map PRIVATE benchmark::benchmark benchmark::benchmark_main)