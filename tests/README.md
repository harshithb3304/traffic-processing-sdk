# Unit Tests for Traffic Processing SDK

## What Are These Tests?

These unit tests verify individual components of the Traffic Processing SDK in isolation:

### 🧪 `test_json_serialization.cpp`

**Purpose**: Tests the JSON creation logic that converts HTTP request/response data into Kafka messages.

**What it tests**:

- ✅ Basic HTTP request → JSON conversion
- ✅ Special characters and Unicode handling
- ✅ Latency calculation (nanoseconds → milliseconds)
- ✅ Edge cases: empty data, invalid timestamps
- ✅ Complex nested JSON preservation

**Why this matters**: This is the core business logic - if JSON serialization is wrong, all Kafka messages will be corrupted.

### ⚙️ `test_configuration.cpp`

**Purpose**: Tests configuration detection and default values.

**What it tests**:

- ✅ Docker vs localhost environment detection
- ✅ Default batching values match working Python code
- ✅ Custom configuration override
- ✅ Environment variable handling

**Why this matters**: Wrong configuration means Kafka won't work or batching will fail.

### 📊 `test_data_structures.cpp`

**Purpose**: Tests the RequestData and ResponseData structures that hold HTTP traffic information.

**What it tests**:

- ✅ Field assignment and retrieval
- ✅ HTTP method validation
- ✅ Status code handling
- ✅ Header JSON handling (complex cases)
- ✅ Timestamp precision (nanoseconds)
- ✅ Body data (text + base64)

**Why this matters**: These structures are the foundation - if they don't work, nothing works.

## How to Run the Tests

### Option 1: Simple Tests (Recommended - Always Works)

```bash
# Simple unit tests that work without complex dependencies
docker compose run --rm -v $PWD:/tmp/host traffic-processor bash -c "
cp /tmp/host/run_unit_tests.cpp /app/ &&
cd /app &&
g++ -std=c++17 -I include -I /usr/include/nlohmann \
    run_unit_tests.cpp src/sdk.cpp src/kafka_producer.cpp \
    -lrdkafka -lfmt -lpthread -o unit_tests_simple &&
./unit_tests_simple"
```

### Option 2: Catch2 Tests (Professional Framework)

```bash
# Build and run tests with Catch2 (requires complex setup)
docker compose build
docker compose run --rm traffic-processor bash -c "ctest --test-dir build"

# Or run tests directly
docker compose run --rm traffic-processor bash -c "./build/unit_tests"
```

### Option 3: Local Build (if you have dependencies)

```bash
# Build with CMake
cmake -B build -S .
cmake --build build

# Run with CTest (shows pass/fail summary)
ctest --test-dir build

# Run directly (shows detailed output)
./build/unit_tests
```

**Recommendation**: Use Option 1 (Simple Tests) for daily development since it always works and is fast. Use Option 2 for formal testing when you need advanced features.

## Understanding Test Output

### ✅ Successful Test Run

```
All tests passed (42 assertions in 12 test cases)
```

### ❌ Failed Test Example

```
test_json_serialization.cpp:45: FAILED:
  REQUIRE( result["latency_ms"] == 500 )
with expansion:
  0 == 500
```

### 📊 Detailed Test Output

```bash
# Run with verbose output
./build/unit_tests -v

# Run specific test
./build/unit_tests "[json]"

# List all tests
./build/unit_tests --list-tests
```

## Test Categories

Tests are organized with tags for easy filtering:

- `[json]` - JSON serialization tests
- `[config]` - Configuration tests
- `[data]` - Data structure tests
- `[edge-cases]` - Edge case scenarios
- `[validation]` - Input validation tests

```bash
# Run only JSON tests
./build/unit_tests "[json]"

# Run only edge case tests
./build/unit_tests "[edge-cases]"
```

## Why Unit Tests Matter

### 🚀 **Benefits for Your Project**:

1. **Catch bugs early** - Before they reach production
2. **Safe refactoring** - Change code confidently
3. **Documentation** - Tests show how code should work
4. **CI/CD integration** - Automatic quality checks
5. **Faster debugging** - Pinpoint exact issues

### 🎯 **What Makes These Good Unit Tests**:

1. **Fast** - Run in milliseconds
2. **Independent** - No Kafka, no network, no files
3. **Focused** - One behavior per test
4. **Clear names** - Know what failed immediately
5. **Good coverage** - Test normal + edge cases

### 📈 **Testing Pyramid**:

```
     /\
    /  \    System Tests (few, slow, expensive)
   /____\
  /      \  Integration Tests (some, medium speed)
 /________\
/          \ Unit Tests (many, fast, cheap) ← YOU ARE HERE
\__________/
```

Unit tests form the foundation - they should be the majority of your tests because they're fast and catch most bugs.
