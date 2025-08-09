cmake_minimum_required(VERSION 3.20)
project(demo_crow_consumer CXX)
set(CMAKE_CXX_STANDARD 17)

find_package(Crow CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

include(FetchContent)
FetchContent_Declare(traffic_processor_sdk
  GIT_REPOSITORY https://github.com/your-org/traffic-processing-sdk.git
  GIT_TAG v0.1.0
)
FetchContent_MakeAvailable(traffic_processor_sdk)

add_executable(crow_demo main.cpp)
target_link_libraries(crow_demo PRIVATE traffic_processor_sdk Crow::Crow)

