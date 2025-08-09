# Traffic Processor SDK (C++)

Minimal SDK and demo server to capture HTTP requests/responses and stream them to a local Kafka topic.

**Note: Demo server currently supports GET and POST methods only on `/echo` endpoint.**

## What this does

- Starts a Crow-based HTTP server on http://localhost:8080
- Logs every request/response
- Batches and sends JSON records to Kafka topic `http.traffic`
- Uses local, plaintext Kafka (no cloud creds needed)

## Prerequisites

- Docker Desktop (or Docker Engine)

## Quick start (Docker demo)

1. Start services (Kafka + server)

```bash
docker compose up --build
```

2. Send requests

```bash
docker exec -it traffic-processor curl -s http://localhost:8080/echo
curl -X POST http://localhost:8080/echo -H "Content-Type: application/json" -d '{"hello":"world"}'
```

3. View application logs

```bash
docker logs -f traffic-processor
```

4. Override config via .env (recommended)

Create a `.env` file in the repo root or copy the example and tweak:

```bash
cp .env.example .env
```

Then restart the container to apply changes:

```bash
docker compose up -d --build
```

4. View Kafka topic messages

```bash
docker exec -it kafka /opt/kafka/bin/kafka-topics.sh --bootstrap-server localhost:19092 --list
docker exec -it kafka /opt/kafka/bin/kafka-console-consumer.sh --bootstrap-server localhost:19092 --topic http.traffic --from-beginning
```

5. Stop everything

```bash
docker compose down
```

## Notes

- Endpoint is `/echo`. Root `/` returns 404 by design.
- The server sends data to Kafka in batches asynchronously. All batching-related
  settings can be overridden via environment variables in `.env` and are passed
  into the container using `env_file`.
- Local Kafka ports: 9092 (host) and 19092 (internal Docker network).
- No credentials are required for this local setup.

## Examples included

- `examples/crow_echo_server/`: Minimal echo server wired with the SDK. This is what the Docker image runs by default. Hitting `/echo` captures request/response and sends to Kafka.
- `examples/crow_consumer_demo/`: Same idea, but kept tiny to showcase SDK-only consumption; suitable as a reference to embed in your own Crow/Drogon/etc. projects.

## Testing

### Unit Tests

Unit tests verify core business logic (JSON creation, configuration, data structures) without needing Kafka or external services. They catch bugs early and run fast.

```bash
docker compose run --rm -v $PWD:/tmp/host traffic-processor bash -c "cp /tmp/host/run_unit_tests.cpp /app/ && cd /app && g++ -std=c++17 -I include -I /usr/include/nlohmann run_unit_tests.cpp src/sdk.cpp src/kafka_producer.cpp -lrdkafka -lfmt -lpthread -o unit_tests_simple && ./unit_tests_simple"
```

### Integration Tests

Test the full HTTP to Kafka pipeline:

```bash
./test_batching.sh
./test.sh
./test_comprehensive.sh
```

## Packaging (what’s in the archive and how to use it)

- The generated archive contains exactly:
  - `include/traffic_processor/*.hpp` (public headers)
  - `lib/libtraffic_processor_sdk.a` (static library)
  - No runtime daemons or tools; this is a middleware SDK meant to be linked into your server.

Build the archive for your OS/arch:

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
cmake --install build --prefix install
cpack --config build/CPackConfig.cmake
```

This produces `traffic-processing-sdk-<version>-<OS>-<arch>.{tar.gz,zip}` at repo root.

Use the archive directly (manual link example):

```bash
tar -xzf traffic-processing-sdk-<version>-<OS>-<arch>.tar.gz
export SDK_PREFIX="$PWD/traffic-processing-sdk-<version>-<OS>-<arch>"
g++ -std=c++17 -I"$SDK_PREFIX/include" your_server.cpp -L"$SDK_PREFIX/lib" -ltraffic_processor_sdk -lrdkafka -lfmt -lpthread -o your_server
```

Or consume as source via CMake (recommended): add the repo with FetchContent and link the `traffic_processor_sdk` target.

## Use as an SDK

- In your C++ server, initialize once and call `capture(request, response)` per request.
- See `examples/crow_echo_server/main.cpp` for a minimal integration.

### Use as middleware in your server (framework-agnostic)

```cpp
#include "traffic_processor/sdk.hpp"
using namespace traffic_processor;

// At startup
SdkConfig cfg;                 // set fields or read env (KAFKA_URL, KAFKA_BATCH_* ...)
cfg.kafka.bootstrapServers = "kafka:19092";
cfg.kafka.topic = "http.traffic";
TrafficProcessorSdk::instance().initialize(cfg);

// Around each request
RequestData r;  /* fill method, path, headers, bodyText/bodyBase64, ip, startNs */
ResponseData s; /* fill status, headers, bodyText/bodyBase64, endNs */
TrafficProcessorSdk::instance().capture(r, s);

// On shutdown
TrafficProcessorSdk::instance().shutdown();
```

Common environment variables (optional):

- `KAFKA_URL` (e.g., `kafka:19092` inside Docker)
- `KAFKA_TOPIC` (default `http.traffic`)
- `KAFKA_BATCH_TIMEOUT` (linger.ms)
- `KAFKA_BATCH_SIZE` (batch.num.messages)
- `KAFKA_BATCH_SIZE_BYTES` (batch.size)
- `KAFKA_BUFFER_MAX_MESSAGES`, `KAFKA_BUFFER_MAX_KBYTES`
- `KAFKA_ACKS`, `KAFKA_COMPRESSION`, `KAFKA_REQUEST_TIMEOUT_MS`

## Build & package SDK via vcpkg (cross‑platform)

### Linux/macOS

```bash
# Prereqs (Linux):
sudo apt-get update && sudo apt-get install -y git cmake build-essential pkg-config

# vcpkg (once)
git clone https://github.com/microsoft/vcpkg $HOME/vcpkg
$HOME/vcpkg/bootstrap-vcpkg.sh -disableMetrics
export VCPKG_ROOT=$HOME/vcpkg

# Clean, configure (SDK only), build, install, package
rm -rf build install *.tar.gz *.zip
cmake -B build -S . \
  -DCMAKE_BUILD_TYPE=Release \
  -DTRAFFIC_SDK_BUILD_EXAMPLES=OFF \
  -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
  -DVCPKG_FEATURE_FLAGS=manifests,versions
cmake --build build --parallel
cmake --install build --prefix install
cpack --config build/CPackConfig.cmake
```

### Windows (PowerShell)

```powershell
# Prereqs: Visual Studio with C++ CMake tools
git clone https://github.com/microsoft/vcpkg $env:USERPROFILE\vcpkg
& $env:USERPROFILE\vcpkg\bootstrap-vcpkg.bat -disableMetrics
$env:VCPKG_ROOT = "$env:USERPROFILE\vcpkg"

Remove-Item -Recurse -Force build, install -ErrorAction SilentlyContinue; Get-ChildItem *.zip,*.tar.gz | Remove-Item -Force -ErrorAction SilentlyContinue
cmake -B build -S . `
  -DCMAKE_BUILD_TYPE=Release `
  -DTRAFFIC_SDK_BUILD_EXAMPLES=OFF `
  -DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake `
  -DVCPKG_FEATURE_FLAGS=manifests,versions
cmake --build build --parallel
cmake --install build --prefix install
cpack --config build/CPackConfig.cmake
```

- Artifacts: `traffic-processing-sdk-<version>-<OS>-<arch>.{tar.gz,zip}` at repo root.
- Installed layout: `install/include`, `install/lib`.
