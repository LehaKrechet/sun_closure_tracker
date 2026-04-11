cmake_minimum_required(VERSION 3.15)
project(Robot LANGUAGES CXX)


include(FetchContent)

# Declare the external library
FetchContent_Declare(
  httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_SHALLOW    TRUE
)
#   GIT_TAG        v0.40 # Use a specific tag, commit hash, or branch
# https://github.com/yhirose/cpp-httplib.git


FetchContent_MakeAvailable(httplib)


FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
)
FetchContent_MakeAvailable(json)

#   GIT_TAG v3.12.0 # Use a specific version tag

add_executable(server src/server.cpp)
add_executable(client src/client.cpp)

target_link_libraries(server PRIVATE httplib)
target_link_libraries(server PRIVATE nlohmann_json::nlohmann_json)

target_link_libraries(client PRIVATE httplib)
target_link_libraries(client PRIVATE nlohmann_json::nlohmann_json)