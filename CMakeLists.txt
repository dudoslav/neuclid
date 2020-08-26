cmake_minimum_required(VERSION 3.12)

project(neuclid)

option(ENABLE_CLANG_TIDY "Enable static analysis with clang-tidy" OFF)

if(ENABLE_CLANG_TIDY)
  set(CMAKE_CXX_CLANG_TIDY clang-tidy)
endif()

find_package(SDL2 REQUIRED)

add_executable(neuclid main.cpp src/game.cpp src/world.cpp src/graphics.cpp)
target_link_libraries(neuclid PRIVATE SDL2)
target_compile_features(neuclid PRIVATE cxx_std_20)
target_compile_options(neuclid PRIVATE -Wall -Wextra -pedantic)
target_include_directories(neuclid PRIVATE include)
