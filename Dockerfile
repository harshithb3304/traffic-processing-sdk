# Fast build using only system packages + header-only Crow
FROM ubuntu:22.04

# Install system packages (fast, no source compilation of deps)
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    pkg-config \
    git \
    librdkafka-dev \
    nlohmann-json3-dev \
    libfmt-dev \
    libasio-dev \
    ca-certificates \
    curl \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Fetch Crow headers (header-only usage, no build step)
RUN git clone --depth 1 https://github.com/CrowCpp/Crow.git /tmp/crow \
 && cp -r /tmp/crow/include/* /usr/local/include/ \
 && rm -rf /tmp/crow

# Copy source files
COPY include/ include/
COPY src/ src/
COPY examples/crow_echo_server/ examples/crow_echo_server/

# Generate a minimal CMake project that builds the SDK + Crow echo server
COPY <<EOF CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(traffic_processor_sdk LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(RDKAFKA REQUIRED rdkafka)

add_library(traffic_processor_sdk
    src/kafka_producer.cpp
    src/sdk.cpp
)

target_include_directories(traffic_processor_sdk PUBLIC 
    \${CMAKE_CURRENT_SOURCE_DIR}/include
    \${RDKAFKA_INCLUDE_DIRS}
)

target_link_libraries(traffic_processor_sdk PUBLIC 
    \${RDKAFKA_LIBRARIES}
    fmt
    pthread
)

target_compile_options(traffic_processor_sdk PUBLIC \${RDKAFKA_CFLAGS_OTHER})

add_executable(crow_echo_server examples/crow_echo_server/main.cpp)

target_link_libraries(crow_echo_server PRIVATE 
    traffic_processor_sdk
    pthread
)
EOF

# Build quickly (header-only Crow + system libs)
RUN cmake -B build -S . && \
    cmake --build build --parallel $(nproc)

# Runtime setup
RUN useradd -r -s /bin/false appuser
USER appuser

ENV DOCKER_ENV=true
EXPOSE 8080

CMD ["./build/crow_echo_server"]