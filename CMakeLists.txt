cmake_minimum_required(VERSION 3.20)

project(traffic_processor_sdk LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Enable folders for IDEs
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# vcpkg manifest mode is assumed (vcpkg.json present in repo)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(RdKafka CONFIG REQUIRED)

option(TRAFFIC_SDK_BUILD_EXAMPLES "Build example servers/binaries" ON)

add_library(traffic_processor_sdk
  src/kafka_producer.cpp
  src/sdk.cpp
)
target_include_directories(traffic_processor_sdk PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(traffic_processor_sdk
  PUBLIC
    nlohmann_json::nlohmann_json
    RdKafka::rdkafka
    fmt::fmt
)

if(TRAFFIC_SDK_BUILD_EXAMPLES)
  find_package(Crow CONFIG REQUIRED)
  add_executable(crow_echo_server examples/crow_echo_server/main.cpp)

  # Platform-specific libraries
  if(WIN32)
      set(PLATFORM_LIBS ws2_32 mswsock)
  else()
      set(PLATFORM_LIBS pthread)
  endif()

  target_link_libraries(crow_echo_server PRIVATE 
      traffic_processor_sdk 
      Crow::Crow
      ${PLATFORM_LIBS}
  )
  set_target_properties(crow_echo_server PROPERTIES FOLDER examples)
  install(TARGETS crow_echo_server)
endif()

install(TARGETS traffic_processor_sdk)

