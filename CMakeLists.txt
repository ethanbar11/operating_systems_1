cmake_minimum_required(VERSION 3.10)  # CMake version check
project(operating_system_1)               # Create project "simple_example"
set(CMAKE_CXX_STANDARD 11)            # Enable c++14 standard


# Add executable target with source files listed in SOURCE_FILES variable
add_executable(operating_system_1 smash.cpp Commands.h Commands.cpp signals.h signals.cpp)
