# Multi-stage Dockerfile for C++ Traffic Processor SDK
FROM ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    curl \
    zip \
    unzip \
    tar \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

# Install vcpkg
WORKDIR /vcpkg
RUN git clone https://github.com/Microsoft/vcpkg.git . && \
    ./bootstrap-vcpkg.sh

# Set up environment for vcpkg
ENV VCPKG_ROOT=/vcpkg
ENV PATH="${VCPKG_ROOT}:${PATH}"

# Copy project files
WORKDIR /app
COPY vcpkg.json vcpkg-configuration.json ./
COPY CMakeLists.txt ./
COPY include/ include/
COPY src/ src/
COPY examples/ examples/

# Install dependencies via vcpkg
RUN vcpkg install --triplet=x64-linux

# Build the project
RUN cmake -B build -S . \
    -DCMAKE_TOOLCHAIN_FILE=/vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build --config Release

# Runtime stage
FROM ubuntu:22.04

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libssl3 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Copy the built executable
COPY --from=builder /app/build/crow_echo_server /usr/local/bin/

# Create non-root user
RUN useradd -r -s /bin/false appuser
USER appuser

# Set environment to indicate Docker environment
ENV DOCKER_ENV=true

EXPOSE 8080

CMD ["crow_echo_server"]
