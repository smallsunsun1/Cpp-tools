cmake_minimum_required(VERSION 3.15)
project(tools)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Release)

option(COMPILE_LIBS OFF)
set(COMPILE_LIBS ON)
if (COMPILE_LIBS)
    add_subdirectory(libs)
endif ()


add_executable(tools main.cpp)
target_link_libraries(tools PRIVATE /Users/sunjiahe/CLionProjects/tools/cmake-build-debug/libs/liblibs.a)