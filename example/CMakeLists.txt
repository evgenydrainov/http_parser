cmake_minimum_required(VERSION 3.7)
project(example)

include_directories(example ../include)

add_executable(example main.c)
target_link_libraries(example http_parser)
