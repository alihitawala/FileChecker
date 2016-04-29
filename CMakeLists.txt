cmake_minimum_required(VERSION 3.3)
project(FileChecker)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror")

set(SOURCE_FILES fscheck.c)
add_executable(FileChecker ${SOURCE_FILES})