cmake_minimum_required(VERSION 2.6)
project(MyFtp)

set(CMAKE_C_FLAGS "-g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories(FileManager)
add_subdirectory(Main)
add_subdirectory(FileManager)

