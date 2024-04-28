cmake_minimum_required(VERSION 3.14)

# Guard against in-source builds
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "In-source builds not allowed. Please make a new directory and run CMake from there. You may need"
                      " to remove CMakeCache.txt.")
endif()

# Options --------------------------------------------------------------------------------------------------------------

option(BUILD_TESTS "Build test executables" OFF)

if(BUILD_TESTS)
  list(APPEND VCPKG_MANIFEST_FEATURES "tests")
endif()

project("LWCLI" VERSION 0.0.1 LANGUAGES CXX)

# Library build settings -----------------------------------------------------------------------------------------------

set(HEADER_DIR ${CMAKE_SOURCE_DIR}/include)
set(LWCLI_PUBLIC_HEADERS
  "options.hpp"
  "cast.hpp"
  "type_utility.hpp"
  "exceptions.hpp"
  "unreachable.hpp"
  "parser.hpp"
  "_options_stores.hpp")
list(TRANSFORM LWCLI_PUBLIC_HEADERS PREPEND include/LWCLI/)

add_library(${PROJECT_NAME} INTERFACE ${LWCLI_PUBLIC_HEADERS})
target_include_directories(${PROJECT_NAME} INTERFACE ${HEADER_DIR})
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)

# Subdirectories -------------------------------------------------------------------------------------------------------

add_subdirectory(sandbox)

if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()