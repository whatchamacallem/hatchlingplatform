# Copyright 2017-2025 Adrian Johnston
#
# A CMake build that uses Google Test instead of the hxTest.h reimplementation.
# This file targets Google Test and is not intended for building a library.

cmake_minimum_required(VERSION 3.10)
project(Hatchling)


file(GLOB SOURCES "src/*.cpp" "src/*.c")
file(GLOB TEST_SOURCES "test/*.cpp")

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

target_include_directories(hxtest SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(hxtest PRIVATE HX_USE_GOOGLE_TEST)
target_compile_options(hxtest PRIVATE -fno-exceptions)


# Download GoogleTest instead of using system version.
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/releases/download/v1.15.2/googletest-1.15.2.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(googletest)

# Use CMake to check that the tests are enumerable.
include(GoogleTest)
gtest_discover_tests(hxtest)
