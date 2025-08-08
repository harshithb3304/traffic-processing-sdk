# Traffic Processor SDK (C++)

A professional HTTP traffic capture and processing SDK that intercepts web server requests/responses and sends them asynchronously to Kafka. Designed for easy integration into any C++ web service.

## 🎯 What It Does

- **Captures HTTP Traffic**: Intercepts all requests and responses from your web server
- **Serializes to JSON**: Converts HTTP data into structured JSON format
- **Batches Messages**: Groups multiple requests for efficient processing  
- **Sends to Kafka**: Asynchronously forwards data to Kafka broker
- **Zero Performance Impact**: Background processing doesn't block your web server

## 🚀 Quick Start

**Prerequisites**: Docker installed and running

> **Note for Windows users**: PowerShell's `curl` is actually `Invoke-WebRequest`. For real curl, install from [curl.se](https://curl.se/windows/) or use the PowerShell commands shown below.

### **Windows**
```cmd
# 1. Start Kafka and HTTP server
.\run.ps1
# OR
start.cmd

# 2. Test the API in another terminal
.\test-simple.ps1

# 3. Watch real-time traffic logs
.\watch-logs.ps1
```

### **Linux/macOS**
```bash
# 1. Start Kafka and HTTP server
./run.sh

# 2. Test the API in another terminal
./test-simple.sh

# 3. Watch real-time traffic logs
./watch-logs.sh
```

## 📁 Project Structure

```
traffic-processor-sdk/
├── 🐳 Docker Setup
│   ├── docker-compose.simple.yml    # Kafka + Python demo server
│   └── Dockerfile.simple            # Python server with traffic capture
├── 🧠 C++ SDK Core
│   ├── include/traffic_processor/
│   │   ├── sdk.hpp                  # Main SDK interface
│   │   └── kafka_producer.hpp       # Kafka client wrapper
│   ├── src/
│   │   ├── sdk.cpp                  # SDK implementation
│   │   └── kafka_producer.cpp       # Kafka producer logic
│   └── examples/crow_echo_server/
│       └── main.cpp                 # Integration example
├── ⚙️ Build System
│   ├── CMakeLists.txt               # C++ build configuration
│   ├── vcpkg.json                   # Dependencies (cppkafka, crow, etc.)
│   └── vcpkg-configuration.json     # vcpkg settings
└── 🔧 Scripts (Cross-Platform)
    ├── run.ps1 / run.sh             # Start everything
    ├── test.ps1 / test.sh           # Comprehensive API tests
    ├── test-simple.ps1 / test-simple.sh  # Quick test commands
    ├── watch-logs.ps1 / watch-logs.sh    # Real-time log monitoring
    └── start.cmd                    # Windows launcher
```

## 🧪 Testing Commands

### **Start Services**

**Windows:**
```cmd
.\run.ps1
# OR
start.cmd
```

**Linux/macOS:**
```bash
./run.sh
```

**Manual (any OS):**
```bash
docker compose -f docker-compose.simple.yml up -d
```

### **Test HTTP Traffic Capture**

**Windows:**
```cmd
# Quick tests
.\test-simple.ps1

# Comprehensive tests  
.\test.ps1
```

**Linux/macOS:**
```bash
# Quick tests
./test-simple.sh

# Comprehensive tests
./test.sh
```

**Manual (any OS using curl):**
```bash
# GET request
curl http://localhost:8080

# POST request with JSON (multi-line)
curl -X POST http://localhost:8080 \
  -H "Content-Type: application/json" \
  -d '{"name":"test","data":"github-demo"}'

# POST request (single line for easy copy-paste)
curl -X POST http://localhost:8080 -H "Content-Type: application/json" -d '{"test":"data"}'
```

### **Monitor Traffic Capture**

**Windows:**
```cmd
.\watch-logs.ps1
```

**Linux/macOS:**
```bash
./watch-logs.sh
```

**Manual (any OS):**
```bash
# Recent logs
docker logs traffic-processor-simple --tail 20

# Follow logs
docker logs -f traffic-processor-simple
```

### **Universal Commands (Works on All OS)**

**Start Services:**
```bash
docker compose -f docker-compose.simple.yml up -d
```

**Test with curl:**

**Linux/macOS/Windows (with real curl installed):**
```bash
# Test GET request
curl http://localhost:8080

# Test POST request
curl -X POST http://localhost:8080 -H "Content-Type: application/json" -d '{"test":"data"}'

# Test with more complex JSON
curl -X POST http://localhost:8080 -H "Content-Type: application/json" -d '{"name":"John","action":"test","timestamp":"2025-01-01"}'
```

**Windows PowerShell (native commands):**
```powershell
# Test GET request
Invoke-RestMethod -Uri "http://localhost:8080" -Method GET

# Test POST request
$body = '{"test":"data"}'
Invoke-RestMethod -Uri "http://localhost:8080" -Method POST -Body $body -ContentType "application/json"

# Test with complex JSON
$body = '{"name":"John","action":"test","timestamp":"2025-01-01"}'
Invoke-RestMethod -Uri "http://localhost:8080" -Method POST -Body $body -ContentType "application/json"
```

**Monitor logs:**
```bash
# View recent logs
docker logs traffic-processor-simple --tail 20

# Follow logs in real-time
docker logs -f traffic-processor-simple
```

**Cleanup:**
```bash
docker compose -f docker-compose.simple.yml down
```

## 🏗️ Architecture

### **Data Flow**
```
HTTP Request → Web Server → TrafficProcessorSdk.capture() → JSON Serialization → Queue → Background Worker → Kafka
```

### **Key Components**

1. **TrafficProcessorSdk** (`src/sdk.cpp`)
   - Main SDK class (singleton pattern)
   - Captures request/response data
   - Queues messages for background processing
   - Manages worker thread for Kafka sending

2. **KafkaProducer** (`src/kafka_producer.cpp`)
   - Kafka client wrapper using `cppkafka`
   - Handles connection to Kafka broker
   - Sends batched JSON messages

3. **Background Worker** (`src/sdk.cpp:workerLoop()`)
   - Runs in separate thread
   - Batches messages by size (100) or timeout (5s)
   - Sends to Kafka asynchronously

### **Integration Example**
```cpp
#include "traffic_processor/sdk.hpp"

// Initialize SDK (once at startup)
TrafficProcessorSdk::instance().initialize();

// In your HTTP handler
CROW_ROUTE(app, "/api").methods("POST"_method)([](const crow::request& req) {
    auto start = std::chrono::steady_clock::now();
    
    // Your business logic here
    crow::response resp = process_request(req);
    
    // Capture traffic (non-blocking)
    RequestData reqData = build_request_data(req, start);
    ResponseData resData = build_response_data(resp);
    TrafficProcessorSdk::instance().capture(reqData, resData);
    
    return resp;
});
```

## 🔧 Configuration

**Current Setup (Local Development)**:
- **Kafka Broker**: `localhost:9092`
- **Topic**: `http.traffic`
- **Account ID**: `local-traffic-processor`
- **Batch Size**: 100 messages
- **Batch Timeout**: 5 seconds
- **Compression**: LZ4

**All configuration is hardcoded for local development. No environment variables needed.**

## 📊 Captured Data Format

```json
{
  "account_id": "local-traffic-processor",
  "timestamp": 1691234567,
  "request": {
    "method": "POST",
    "scheme": "http",
    "host": "localhost:8080",
    "path": "/api/users",
    "query": "?filter=active",
    "headers": {"Content-Type": "application/json"},
    "body_b64": "eyJuYW1lIjoiSm9obiJ9",
    "ip": "192.168.1.100"
  },
  "response": {
    "status": 200,
    "headers": {"Content-Type": "application/json"},
    "body_b64": "eyJpZCI6MTIzLCJuYW1lIjoiSm9obiJ9"
  },
  "latency_ms": 45
}
```

## 🛠️ Development

### **C++ SDK Development**
```bash
# Install dependencies
vcpkg install cppkafka crow nlohmann-json fmt

# Build
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg-root]/scripts/buildsystems/vcpkg.cmake
cmake --build .
```

### **Dependencies**
- **cppkafka**: Kafka client library
- **nlohmann-json**: JSON serialization
- **crow**: Web framework (for example server)
- **fmt**: String formatting

## 🎯 Use Cases

- **API Traffic Analysis**: Monitor all API calls and responses
- **Performance Monitoring**: Track request latency and throughput
- **Security Auditing**: Log all HTTP traffic for security analysis
- **Debugging**: Capture complete request/response data for troubleshooting
- **Analytics Pipeline**: Feed HTTP data into data processing systems

## 📚 Inspired By

- [Akto SDK Documentation](https://docs.akto.io/traffic-connector/akto-sdk)
- [Flask Middleware](https://github.com/akto-api-security/flask-middleware)
- [Express API Logging](https://github.com/akto-api-security/express-api-logging)
- [Go Middleware](https://github.com/akto-api-security/gomiddleware)

## ✅ Cross-Platform Ready

This repository works on **Windows, Linux, and macOS** with:
- ✅ **Cross-Platform Scripts**: Both `.ps1` (Windows) and `.sh` (Unix) versions
- ✅ **Docker-Based**: Consistent behavior across all operating systems
- ✅ **No OS Dependencies**: Everything runs in containers
- ✅ **Universal Commands**: `curl` and `docker` work everywhere
- ✅ **Complete Documentation**: OS-specific instructions included

### **Quick Test Summary**

| Platform | Start | Test | Monitor |
|----------|-------|------|---------|
| **Windows** | `.\run.ps1` | `.\test-simple.ps1` | `.\watch-logs.ps1` |
| **Linux/macOS** | `./run.sh` | `./test-simple.sh` | `./watch-logs.sh` |
| **Manual (any OS)** | `docker compose -f docker-compose.simple.yml up -d` | See "Universal Commands" section above | `docker logs -f traffic-processor-simple` |

**Ready to push to GitHub and work on any developer's machine!** 🌍