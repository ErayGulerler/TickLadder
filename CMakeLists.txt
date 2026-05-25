cmake_minimum_required(VERSION 3.20)
project(TickLadder VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# Keep the commands in the README identical for single- and multi-config generators.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "$<1:${CMAKE_BINARY_DIR}/bin>")

add_library(tickladder INTERFACE)
target_include_directories(tickladder INTERFACE include)
if(MSVC)
  target_compile_options(tickladder INTERFACE /W4 /permissive-)
else()
  target_compile_options(tickladder INTERFACE -Wall -Wextra -Wpedantic)
endif()

add_executable(replay src/replay.cpp src/main.cpp)
target_link_libraries(replay PRIVATE tickladder)

add_executable(book_bench bench/benchmark.cpp)
target_link_libraries(book_bench PRIVATE tickladder)
target_include_directories(book_bench PRIVATE support)

include(CTest)
if(BUILD_TESTING)
  add_executable(book_tests tests/book_tests.cpp src/replay.cpp)
  target_link_libraries(book_tests PRIVATE tickladder)
  target_include_directories(book_tests PRIVATE support)
  add_test(NAME book_tests COMMAND book_tests)
  add_test(NAME replay_example COMMAND ${CMAKE_COMMAND}
    -DPROGRAM=$<TARGET_FILE:replay> -DDATA=${CMAKE_CURRENT_SOURCE_DIR}/examples
    -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/check_example.cmake)
endif()
