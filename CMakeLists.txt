# Copyright 2017-2025 Adrian Johnston
#
# A CMake build that uses Google Test instead of the hxTest.h reimplementation.


cmake_minimum_required(VERSION 3.10)
project(Hatchling)

set(CMAKE_CXX_STANDARD 11)

file(GLOB SOURCES "src/*.cpp" "src/*.c")

file(GLOB TEST_SOURCES "test/*.cpp")

include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/include)
add_compile_definitions(HX_USE_GOOGLE_TEST)

# Add GoogleTest
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/releases/download/v1.15.2/googletest-1.15.2.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# Add the executable for tests
add_executable(
  hxTests
  ${SOURCES}
  ${TEST_SOURCES}
)

target_link_libraries(
  hxTests
  gtest
)

include(GoogleTest)
gtest_discover_tests(hxTests)
