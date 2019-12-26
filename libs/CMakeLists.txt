cmake_minimum_required(VERSION 3.15)

add_library(libs work_queue.h stl_thread_pool.cpp stl_thread_pool.h macros.h
        thread_safe_structures.h thread_safe_structures.cpp env.h env.cpp)