# Traffic Processor SDK (C++)

Minimal SDK and demo server to capture HTTP requests/responses and stream them to a local Kafka topic.

## What this does
- Starts a Crow-based HTTP server on http://localhost:8080
- Logs every request/response
- Batches and sends JSON records to Kafka topic `http.traffic`
- Uses local, plaintext Kafka (no cloud creds needed)

## Prerequisites
- Docker Desktop (or Docker Engine)

## Setup Instructions

1) Start services (Kafka + server)
```bash
docker compose up --build
```

2) Send requests
```bash
# GET
docker exec -it traffic-processor curl -s http://localhost:8080/echo

# POST (on your host)
curl -X POST http://localhost:8080/echo \
  -H "Content-Type: application/json" \
  -d '{"hello":"world"}'
```

3) View application logs
```bash
docker logs -f traffic-processor
```

4) View Kafka topic messages (inside the Kafka container)
```bash
# List topics
docker exec -it kafka /opt/kafka/bin/kafka-topics.sh --bootstrap-server localhost:19092 --list

# Consume the http.traffic topic
docker exec -it kafka /opt/kafka/bin/kafka-console-consumer.sh \
  --bootstrap-server localhost:19092 \
  --topic http.traffic \
  --from-beginning
```

5) Stop everything
```bash
docker compose down
```

## Notes
- Endpoint is `/echo`. Root `/` returns 404 by design.
- The server sends data to Kafka in batches asynchronously.
- Local Kafka ports: 9092 (host) and 19092 (internal Docker network).
- No credentials are required for this local setup.

## Use as an SDK
- In your C++ server, initialize once and call `capture(request, response)` per request.
- See `examples/crow_echo_server/main.cpp` for a minimal integration.