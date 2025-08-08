# Traffic Processor SDK

A high-performance C++ SDK for capturing, processing, and streaming HTTP traffic data to Apache Kafka. This SDK provides a lightweight, non-intrusive way to monitor web application traffic in real-time.

## ✨ Features

- **Zero-Configuration Setup**: Works out of the box with sensible defaults
- **High Performance**: Asynchronous processing with configurable batching
- **Cross-Platform**: Runs on Linux, Windows, and macOS
- **Docker Ready**: Full containerization support for easy deployment
- **Framework Agnostic**: Integrate with any C++ web framework (Crow, Drogon, etc.)
- **Production Ready**: Thread-safe, memory efficient, and fault tolerant

## 🚀 Quick Start

### Using Docker (Recommended)

1. **Start the system:**
   ```bash
   docker compose up --build
   ```

2. **Test the API:**
   ```bash
   # Simple GET request
   curl http://localhost:8080/echo
   
   # POST request with JSON data
   curl -X POST http://localhost:8080/echo \
     -H "Content-Type: application/json" \
     -d '{"message": "Hello Traffic Processor!"}'
   ```

3. **Monitor logs:**
   ```bash
   # Watch traffic processor logs
   docker logs -f traffic-processor
   
   # Watch Kafka logs
   docker logs -f kafka
   ```

### Local Development

1. **Prerequisites:**
   - C++20 compatible compiler
   - CMake 3.20+
   - vcpkg package manager

2. **Install dependencies:**
   ```bash
   vcpkg install
   ```

3. **Build:**
   ```bash
   cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[vcpkg-root]/scripts/buildsystems/vcpkg.cmake
   cmake --build build --config Release
   ```

4. **Run:**
   ```bash
   ./build/crow_echo_server
   ```

## 🔧 Integration

### Basic Integration

```cpp
#include "traffic_processor/sdk.hpp"

int main() {
    // Initialize the SDK
    traffic_processor::TrafficProcessorSdk::instance().initialize();
    
    // Your web server setup here...
    
    // On each HTTP request/response:
    traffic_processor::RequestData request;
    request.method = "POST";
    request.path = "/api/users";
    request.headers = /* your headers */;
    request.bodyBase64 = /* base64 encoded body */;
    
    traffic_processor::ResponseData response;
    response.status = 200;
    response.headers = /* response headers */;
    response.bodyBase64 = /* base64 encoded response */;
    
    // Capture the traffic (non-blocking)
    traffic_processor::TrafficProcessorSdk::instance().capture(request, response);
    
    return 0;
}
```

### Framework-Specific Examples

#### Crow Framework
See `examples/crow_echo_server/main.cpp` for a complete integration example.

#### Drogon Framework
```cpp
// In your Drogon controller
void YourController::handleRequest(const HttpRequestPtr& req, 
                                  std::function<void(const HttpResponsePtr&)>&& callback) {
    // Process request...
    auto response = HttpResponse::newHttpResponse();
    
    // Capture traffic
    RequestData r = buildRequestData(req);
    ResponseData s = buildResponseData(response);
    TrafficProcessorSdk::instance().capture(r, s);
    
    callback(response);
}
```

## 📊 How It Works

```
┌─────────────┐    ┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│   HTTP      │    │   Traffic   │    │   Apache    │    │ Analytics   │
│   Request   │───▶│ Processor   │───▶│   Kafka     │───▶│   System    │
│             │    │    SDK      │    │             │    │             │
└─────────────┘    └─────────────┘    └─────────────┘    └─────────────┘
```

1. **Capture**: SDK intercepts HTTP requests and responses
2. **Serialize**: Data is converted to structured JSON format
3. **Batch**: Messages are batched for efficiency (configurable size/timeout)
4. **Stream**: Batches are sent asynchronously to Kafka
5. **Consume**: Analytics systems process the traffic data

## 🔍 Generated Data Format

```json
{
  "account_id": "local-traffic-processor",
  "timestamp": 1703234567,
  "request": {
    "method": "POST",
    "scheme": "http",
    "host": "localhost:8080",
    "path": "/echo",
    "query": "",
    "headers": {
      "content-type": "application/json",
      "user-agent": "curl/7.81.0"
    },
    "body_b64": "eyJ0ZXN0IjoiZGF0YSJ9",
    "ip": "172.17.0.1"
  },
  "response": {
    "status": 200,
    "headers": {
      "content-type": "application/json"
    },
    "body_b64": "eyJtZXNzYWdlIjoiZWNobyIsIm1ldGhvZCI6IlBPU1QifQ=="
  },
  "latency_ms": 2
}
```

## ⚙️ Configuration

The SDK uses intelligent defaults that work out of the box:

```cpp
struct SdkConfig {
    std::string accountId{"local-traffic-processor"};
    std::size_t batchSize{100};           // Messages per batch
    int batchTimeoutMs{5000};             // Max wait time for batch
    KafkaConfig kafka;                    // Kafka settings
};

struct KafkaConfig {
    std::string bootstrapServers;         // Auto-detected: localhost:9092 or kafka:19092
    std::string topic{"http.traffic"};
    std::string compression{"lz4"};
    int lingerMs{10};                     // Producer batching delay
    int batchKb{512};                     // Producer batch size
};
```

### Environment Detection

The SDK automatically detects its environment:
- **Docker**: Uses `kafka:19092` (internal Docker network)
- **Local**: Uses `localhost:9092` (local Kafka instance)

## 🏗️ Project Structure

```
traffic-processor-sdk/
├── include/traffic_processor/     # Public SDK headers
│   ├── sdk.hpp                   # Main SDK interface
│   └── kafka_producer.hpp        # Kafka integration
├── src/                          # Implementation
│   ├── sdk.cpp                   # Core SDK logic
│   └── kafka_producer.cpp        # Kafka producer
├── examples/                     # Integration examples
│   └── crow_echo_server/         # Crow framework example
├── docker-compose.yml            # Complete Docker setup
├── Dockerfile                    # Multi-stage C++ build
├── CMakeLists.txt               # Build configuration
└── vcpkg.json                   # Dependencies
```

## 🧪 Testing Commands

```bash
# Start the system
docker compose up --build

# Basic tests
curl http://localhost:8080/echo
curl -X POST http://localhost:8080/echo -d '{"test": "data"}' -H 'Content-Type: application/json'

# Performance test
for i in {1..10}; do
  curl -X POST http://localhost:8080/echo -d "{\"test\": \"message_$i\"}" -H 'Content-Type: application/json'
done

# Monitor real-time logs
docker logs -f traffic-processor
```

## 🐳 Docker Architecture

- **Kafka Container**: Apache Kafka in KRaft mode (no Zookeeper needed)
- **Traffic Processor**: C++ application with SDK integration
- **Automatic Networking**: Containers communicate via Docker internal network
- **Health Checks**: Ensures Kafka is ready before starting the processor

## 📈 Performance Characteristics

- **Latency**: < 1ms overhead per request (batched processing)
- **Throughput**: Handles thousands of requests per second
- **Memory**: Configurable batch sizes to control memory usage
- **CPU**: Minimal impact due to asynchronous processing
- **Network**: Efficient compression and batching reduces Kafka load

## 🛠️ Development

### Building Locally
```bash
# Install dependencies
vcpkg install

# Build
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[vcpkg-root]/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

### Debugging
```bash
# Build with debug symbols
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=[vcpkg-root]/scripts/buildsystems/vcpkg.cmake
cmake --build build

# Run with gdb
gdb ./build/crow_echo_server
```

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests
5. Submit a pull request

## 📄 License

This project is licensed under the MIT License - see the LICENSE file for details.

## 🔗 Related Projects

- [Flask Middleware](https://github.com/akto-api-security/flask-middleware)
- [Express API Logging](https://github.com/akto-api-security/express-api-logging)
- [Go Middleware](https://github.com/akto-api-security/gomiddleware)

---

**Built with ❤️ for high-performance traffic monitoring**