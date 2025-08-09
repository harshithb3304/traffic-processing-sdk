cmake_minimum_required(VERSION 3.20)

project(traffic_processor_sdk LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Enable folders for IDEs
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# vcpkg manifest mode is assumed (vcpkg.json present in repo) but we provide
# robust fallbacks using FetchContent and pkg-config so hosts without vcpkg can build.

include(FetchContent)

# nlohmann_json (header-only) – try system/vcpkg first, else fetch
find_package(nlohmann_json CONFIG QUIET)
if(NOT nlohmann_json_FOUND)
  message(STATUS "nlohmann_json not found, fetching release...")
  FetchContent_Declare(json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
  FetchContent_MakeAvailable(json)
endif()

# fmt – try system/vcpkg first, else fetch
find_package(fmt CONFIG QUIET)
if(NOT fmt_FOUND)
  message(STATUS "fmt not found, fetching release...")
  set(FMT_INSTALL OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(fmtproj
    URL https://github.com/fmtlib/fmt/archive/refs/tags/10.2.1.tar.gz)
  FetchContent_MakeAvailable(fmtproj)
endif()

# librdkafka – prefer CMake package; fallback to pkg-config
find_package(RdKafka CONFIG QUIET)
if(NOT RdKafka_FOUND)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(RDKAFKA REQUIRED rdkafka)
endif()

option(TRAFFIC_SDK_BUILD_EXAMPLES "Build example servers/binaries" OFF)

add_library(traffic_processor_sdk
  src/kafka_producer.cpp
  src/sdk.cpp
)
target_include_directories(traffic_processor_sdk PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(traffic_processor_sdk
  PUBLIC
    nlohmann_json::nlohmann_json
    fmt::fmt
)

if(RdKafka_FOUND)
  target_link_libraries(traffic_processor_sdk PUBLIC RdKafka::rdkafka)
else()
  target_include_directories(traffic_processor_sdk PUBLIC ${RDKAFKA_INCLUDE_DIRS})
  target_link_libraries(traffic_processor_sdk PUBLIC ${RDKAFKA_LIBRARIES})
endif()

if(TRAFFIC_SDK_BUILD_EXAMPLES)
  # Crow – try system/vcpkg first, else fetch headers
  find_package(Crow CONFIG QUIET)
  if(NOT Crow_FOUND)
    message(STATUS "Crow not found, fetching v1.1.0 headers...")
    FetchContent_Declare(crow
      URL https://github.com/CrowCpp/Crow/archive/refs/tags/v1.1.0.tar.gz)
    FetchContent_MakeAvailable(crow)
  endif()
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

# Install public headers for SDK consumers
install(DIRECTORY include/ DESTINATION include)

# Simple CPack packaging to produce a single archive per OS (TGZ and ZIP)
set(CPACK_PACKAGE_NAME "traffic-processing-sdk")
set(CPACK_PACKAGE_VENDOR "traffic-processor")
set(CPACK_PACKAGE_VERSION "0.1.0")
set(CPACK_MONOLITHIC_INSTALL ON)
set(CPACK_GENERATOR "TGZ;ZIP")
set(CPACK_PACKAGE_FILE_NAME
    "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
include(CPack)

