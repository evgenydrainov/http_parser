cmake_minimum_required(VERSION 3.6)
project(example)

set(CMAKE_BUILD_TYPE Debug)

include_directories(example ../include)

add_executable(example main.c)
target_link_libraries(example ../../build/${CMAKE_BUILD_TYPE}/http_parser)
