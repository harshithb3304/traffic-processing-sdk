cmake_minimum_required(VERSION 3.20)

project(traffic_processor_sdk LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Enable folders for IDEs
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# vcpkg manifest mode is assumed (vcpkg.json present in repo)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(CppKafka CONFIG REQUIRED)
find_package(Crow CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

add_library(traffic_processor_sdk
  src/kafka_producer.cpp
  src/sdk.cpp
)
target_include_directories(traffic_processor_sdk PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(traffic_processor_sdk
  PUBLIC
    nlohmann_json::nlohmann_json
    CppKafka::cppkafka
    RdKafka::rdkafka
    fmt::fmt
)

add_executable(crow_echo_server examples/crow_echo_server/main.cpp)
target_link_libraries(crow_echo_server PRIVATE 
    traffic_processor_sdk 
    Crow::Crow
    ws2_32
    mswsock
)
set_target_properties(crow_echo_server PROPERTIES FOLDER examples)

install(TARGETS traffic_processor_sdk crow_echo_server)

