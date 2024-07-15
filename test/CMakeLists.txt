cmake_minimum_required(VERSION 3.7)
project(test)

enable_testing()

include_directories(test ../include)

add_executable(test main.c)
target_link_libraries(test http_parser)

add_test(NAME test1 COMMAND test)
