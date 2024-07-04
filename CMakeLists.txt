cmake_minimum_required(VERSION 3.6)
project(http_parser)

set(CMAKE_BUILD_TYPE Debug)

include_directories(http_parser include)

add_library(http_parser STATIC src/http_parser.c)
