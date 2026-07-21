cmake_minimum_required(VERSION 3.20)
project(LoggerDemo)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "" FORCE)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# --- library: Logger itself, no main() ---
add_library(logger src/logger.cpp src/sinks.cpp)
target_include_directories(logger PUBLIC include)

# --- app: main.cpp + the library ---
add_executable(logger_demo main.cpp)
target_link_libraries(logger_demo PRIVATE logger)

# --- tests: your fixture-based TEST_F cases + the library ---
add_executable(unit_tests tests/logger_test.cpp)
target_link_libraries(unit_tests PRIVATE logger GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(unit_tests)

option(ENABLE_ASAN "Build with AddressSanitizer + UndefinedBehaviorSanitizer" OFF)
option(ENABLE_TSAN "Build with ThreadSanitizer" OFF)

if(ENABLE_ASAN AND ENABLE_TSAN)
  message(FATAL_ERROR "ENABLE_ASAN and ENABLE_TSAN cannot be combined in the same binary")
endif()

if(ENABLE_ASAN)
  add_compile_options(-fsanitize=address,undefined -g)
  add_link_options(-fsanitize=address,undefined)
endif()

if(ENABLE_TSAN)
  add_compile_options(-fsanitize=thread -g)
  add_link_options(-fsanitize=thread)
endif()
