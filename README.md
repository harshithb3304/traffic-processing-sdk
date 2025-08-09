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

## Build and package the SDK (run from repo root)

Pick ONE path. Both produce the same SDK outputs.

- Option A: If you already have deps (librdkafka/fmt/json) installed

  ```bash
  cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DTRAFFIC_SDK_BUILD_EXAMPLES=OFF
  cmake --build build --parallel
  cmake --install build --prefix install
  cpack --config build/CPackConfig.cmake
  ```

- Option B: If you want deps resolved automatically (vcpkg)
  ```bash
  git clone https://github.com/microsoft/vcpkg "$HOME/vcpkg" && "$HOME/vcpkg/bootstrap-vcpkg.sh" -disableMetrics
  export VCPKG_ROOT=$HOME/vcpkg
  cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DTRAFFIC_SDK_BUILD_EXAMPLES=OFF \
    -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake -DVCPKG_FEATURE_FLAGS=manifests,versions
  cmake --build build --parallel
  cmake --install build --prefix install
  cpack --config build/CPackConfig.cmake
  ```

### Outputs

- install/include/traffic_processor/\*.hpp
- install/lib/libtraffic_processor_sdk.a
- traffic-processing-sdk-<version>-<OS>-<arch>.{tar.gz,zip} (same content as install/)

## How to use (follow the example)

- Open `examples/crow_echo_server/main.cpp` and mirror the pattern: initialize once at startup, call `capture(r, s)` per request, shutdown on exit.
- Link the SDK in your app by either:
  - Adding this repo via CMake FetchContent and `target_link_libraries(your_app PRIVATE traffic_processor_sdk)`, or
  - Unpacking the archive and adding `-I<archive>/include` and linking `-L<archive>/lib -ltraffic_processor_sdk` (plus RdKafka/fmt if your toolchain needs it).

That’s it. The SDK is framework‑agnostic and works with Crow, Drogon, Oat++, Pistache, Boost.Beast, etc.
