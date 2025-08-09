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

## Setup Instructions

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
- The server sends data to Kafka in batches asynchronously.
- Local Kafka ports: 9092 (host) and 19092 (internal Docker network).
- No credentials are required for this local setup.

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

## Use as an SDK

- In your C++ server, initialize once and call `capture(request, response)` per request.
- See `examples/crow_echo_server/main.cpp` for a minimal integration.

### Parameter-based integration (recommended)

The SDK is configured via parameters. Provide only what you need; everything else uses sensible defaults.

```cpp
#include "traffic_processor/sdk.hpp"
using namespace traffic_processor;

SdkConfig cfg;
cfg.accountId = "your-account-id";                 // optional
cfg.kafka.bootstrapServers = "broker:9092";        // set any you need
cfg.kafka.topic = "http.traffic";                  // optional
cfg.kafka.compression = "lz4";                     // optional
cfg.kafka.acks = "1";                               // optional

// Override any librdkafka property (advanced)
cfg.kafka.extraProperties["linger.ms"] = "10000";            // batch timeout
cfg.kafka.extraProperties["batch.num.messages"] = "100";     // batch size (messages)
cfg.kafka.extraProperties["batch.size"] = "32768";           // batch size (bytes)

TrafficProcessorSdk::instance().initialize(cfg);
```

- Any field you omit falls back to defaults (local Kafka, topic `http.traffic`, batching enabled).
- Alternatively, you can use environment variables (e.g., `KAFKA_URL`, `KAFKA_TOPIC`). The Crow example loads a `.env` file automatically at startup.
