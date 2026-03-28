cmake_minimum_required(VERSION 3.14)

project(sun_closure_tracker
    LANGUAGES CXX
    VERSION 1.0.0)

set(CMAKE_CXX_STANDART 17)
set(CMAKE_CXX_STANDART_REQUIRED ON)
find_package(OpenCV REQUIRED)

add_executable(sun_closure_traker
    src/app.cpp
    src/main.cpp
    src/engine.cpp
    src/recognizer.cpp
    src/solver.cpp
    src/receiver.cpp)

target_include_directories(sun_closure_traker PRIVATE include ${opencv_SOURCE_DIR}/include ${opencv_BINARY_DIR})
target_link_libraries(sun_closure_traker PRIVATE
    opencv_core
    opencv_imgproc
    opencv_highgui
    opencv_imgcodecs
)
