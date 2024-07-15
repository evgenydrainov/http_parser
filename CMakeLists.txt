cmake_minimum_required(VERSION 3.7)
project(http_parser)

add_subdirectory(test)

include_directories(http_parser include)

add_library(http_parser STATIC src/http_parser.c)
