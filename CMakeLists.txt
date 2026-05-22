cmake_minimum_required(VERSION 3.14)
include(FetchContent)

project(sun_closure_tracker
    LANGUAGES CXX
    VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(OpenCV REQUIRED)

FetchContent_Declare(
  httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_SHALLOW    TRUE
)
set(HTTPLIB_USE_NON_BLOCKING_GETADDRINFO OFF)
FetchContent_MakeAvailable(httplib)


FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
)
FetchContent_MakeAvailable(json)

add_executable(sun_closure_traker
    src/app.cpp
    src/main.cpp
    src/engine.cpp
    src/recognizer.cpp
    src/solver.cpp
    src/receiver.cpp
    src/sendler.cpp)

target_include_directories(sun_closure_traker PRIVATE include ${opencv_SOURCE_DIR}/include ${opencv_BINARY_DIR})
target_link_libraries(sun_closure_traker PRIVATE
    opencv_core
    opencv_imgproc
    opencv_highgui
    opencv_imgcodecs
    opencv_video
)

target_link_libraries(sun_closure_traker PRIVATE httplib)
target_link_libraries(sun_closure_traker PRIVATE nlohmann_json::nlohmann_json)

