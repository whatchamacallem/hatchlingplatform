# SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
# SPDX-License-Identifier: MIT
# This file is licensed under the terms of the LICENSE.md file.
#
# This is a CMake build that uses Google Test instead of the hxTest.h
# reimplementation. It is significantly slower than the test scripts when doing
# a clean build.
#
# This file is for testing and is not intended for building a library for your
# application. There is no expected configuration for use with Hatchling Platform
# as it is expected to work everywhere with all warnings and errors enabled.
# You should be able to add this code to your existing build with no problems.

cmake_minimum_required(VERSION 3.10)
project(Hatchling)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Enable compile database generation")

file(GLOB SOURCES "src/*.c" "src/*.cpp")
file(GLOB TEST_SOURCES "test/*.c" "test/*.cpp")

# Add the executable for tests
add_executable(
  hxtest
  ${SOURCES}
  ${TEST_SOURCES}
)

target_link_libraries(
  hxtest
  gtest
)

target_include_directories(hxtest PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(hxtest PRIVATE HX_USE_GOOGLE_TEST)
target_compile_options(
  hxtest
  PRIVATE
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-fno-exceptions>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-ggdb3>
)

# Download GoogleTest into a local workspace cache to avoid AI sandbox issues.
# CMake will still have to be run once to set up the cache after running
# ./clean.sh.
set(
  HX_FETCHCONTENT_CACHE_DIR
  "${CMAKE_SOURCE_DIR}/cache"
)
file(MAKE_DIRECTORY "${HX_FETCHCONTENT_CACHE_DIR}")
set(
  FETCHCONTENT_BASE_DIR
  "${HX_FETCHCONTENT_CACHE_DIR}"
  CACHE PATH "FetchContent base directory"
  FORCE
)
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/releases/download/v1.15.2/googletest-1.15.2.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "Link GoogleTest with shared CRT on MSVC" FORCE)
FetchContent_MakeAvailable(googletest)

# Use CMake to check that the tests are enumerable.
include(GoogleTest)
gtest_discover_tests(hxtest)
