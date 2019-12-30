cmake_minimum_required(VERSION 3.15)
project(tools)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Release)

option(COMPILE_LIBS OFF)
set(COMPILE_LIBS ON)
if (COMPILE_LIBS)
    add_subdirectory(libs)
endif ()

add_subdirectory(pybind11)
add_executable(tools main.cpp)
target_link_libraries(tools PRIVATE /Users/sunjiahe/CLionProjects/tools/cmake-build-debug/libs/liblibs.a)
add_library(example MODULE example.cpp)
target_link_libraries(example PRIVATE pybind11::module)
target_link_libraries(example PRIVATE libs)
target_include_directories(example PRIVATE /usr/local/Cellar/eigen/3.3.7/include/eigen3)
set_target_properties(example PROPERTIES PREFIX "${PYTHON_MODULE_PREFIX}"
        SUFFIX "${PYTHON_MODULE_EXTENSION}")
