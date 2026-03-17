cmake_minimum_required(VERSION 3.25)

project(json)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES src/*.cxx)
add_library(json ${SOURCES})
target_include_directories(json PUBLIC inc)
