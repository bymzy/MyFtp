cmake_minimum_required(VERSION 2.6)
project(MyFtp)

set(CMAKE_C_FLAGS "-g -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)


include_directories(FileManager)
add_subdirectory(Main)
add_subdirectory(FileManager)

