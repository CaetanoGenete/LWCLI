cmake_minimum_required(VERSION 3.14)

# Guard against in-source builds
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "In-source builds not allowed. Please make a new directory "
                      "and run CMake from there. You may need to remove CMakeCache.txt.")
endif()

project("LWCLI" LANGUAGES CXX)

# Library build settings -----------------------------------------------------------------------------------------------

set(HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(LWCLI_PUBLIC_HEADERS "LWCLI.hpp")
list(TRANSFORM LWCLI_PUBLIC_HEADERS PREPEND ${HEADER_DIR}/LWCLI/)

add_library(${PROJECT_NAME} INTERFACE ${LWCLI_PUBLIC_HEADERS})
target_include_directories(${PROJECT_NAME} INTERFACE ${HEADER_DIR})
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)

# Other sub-directories ------------------------------------------------------------------------------------------------

add_subdirectory(sandbox)

option(BUILD_BENCHMARK "Build benchmark executables" OFF)
if(BUILD_BENCHMARK)
  add_subdirectory(benchmark)
endif()