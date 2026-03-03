cmake_minimum_required(VERSION 3.14)


project(sun_closure_tracker
    LANGUAGES CXX
    VERSION 1.0.0)

add_executable(sun_closure_traker
    src/main.cpp
    src/sun.cpp
    src/cloud.cpp
    src/engine.cpp
    src/image.cpp
    src/solver.cpp)

target_include_directories(sun_closure_traker PRIVATE include)