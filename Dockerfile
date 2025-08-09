FROM ubuntu:22.04
ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake pkg-config \
    librdkafka-dev nlohmann-json3-dev libfmt-dev libasio-dev \
    ca-certificates curl && rm -rf /var/lib/apt/lists/*

WORKDIR /app
# Pin Crow headers without git
RUN curl -sSL https://github.com/CrowCpp/Crow/archive/refs/tags/v1.1.0.tar.gz \
  | tar -xz -C /usr/local/include --strip-components=2 Crow-1.1.0/include

COPY include/ include/
COPY src/ src/
COPY examples/crow_echo_server/ examples/crow_echo_server/
COPY examples/crow_consumer_demo/ examples/crow_consumer_demo/

COPY <<EOF CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(traffic_processor_sdk LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
find_package(PkgConfig REQUIRED)
pkg_check_modules(RDKAFKA REQUIRED rdkafka)
add_library(traffic_processor_sdk src/kafka_producer.cpp src/sdk.cpp)
target_include_directories(traffic_processor_sdk PUBLIC \${CMAKE_CURRENT_SOURCE_DIR}/include \${RDKAFKA_INCLUDE_DIRS})
target_link_libraries(traffic_processor_sdk PUBLIC \${RDKAFKA_LIBRARIES} fmt pthread)
add_executable(crow_echo_server examples/crow_echo_server/main.cpp)
target_link_libraries(crow_echo_server PRIVATE traffic_processor_sdk pthread)
add_executable(crow_consumer_demo examples/crow_consumer_demo/main.cpp)
target_link_libraries(crow_consumer_demo PRIVATE traffic_processor_sdk pthread)
EOF

RUN cmake -B build -S . && cmake --build build --parallel $(nproc)

ENV DOCKER_ENV=true
EXPOSE 8080
CMD ["./build/crow_echo_server"]