cmake_minimum_required(VERSION 3.10)
project(main)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(cpprestsdk_DIR /usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}/cmake/)
find_package(cpprestsdk REQUIRED)

add_executable(main main.cpp metronome.cpp)
target_link_libraries(main PRIVATE cpprest pigpio rt crypto boost_system)
