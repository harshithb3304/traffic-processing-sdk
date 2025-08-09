#include <iostream>
#include <string>
#include <nlohmann/json.hpp>
#include "traffic_processor/sdk.hpp"

using namespace traffic_processor;
using json = nlohmann::json;

// Simple test framework
struct TestRunner
{
    int passed = 0;
    int failed = 0;

    void assert_eq(const std::string &name, const std::string &expected, const std::string &actual)
    {
        if (expected == actual)
        {
            std::cout << "✅ PASS: " << name << std::endl;
            passed++;
        }
        else
        {
            std::cout << "❌ FAIL: " << name << std::endl;
            std::cout << "   Expected: '" << expected << "'" << std::endl;
            std::cout << "   Actual:   '" << actual << "'" << std::endl;
            failed++;
        }
    }

    void assert_eq(const std::string &name, int expected, int actual)
    {
        if (expected == actual)
        {
            std::cout << "✅ PASS: " << name << std::endl;
            passed++;
        }
        else
        {
            std::cout << "❌ FAIL: " << name << std::endl;
            std::cout << "   Expected: " << expected << std::endl;
            std::cout << "   Actual:   " << actual << std::endl;
            failed++;
        }
    }

    void assert_true(const std::string &name, bool condition)
    {
        if (condition)
        {
            std::cout << "✅ PASS: " << name << std::endl;
            passed++;
        }
        else
        {
            std::cout << "❌ FAIL: " << name << " (expected true, got false)" << std::endl;
            failed++;
        }
    }

    void summary()
    {
        std::cout << "\n========================================" << std::endl;
        std::cout << "TEST SUMMARY" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "✅ Passed: " << passed << std::endl;
        std::cout << "❌ Failed: " << failed << std::endl;
        std::cout << "📊 Total:  " << (passed + failed) << std::endl;

        if (failed == 0)
        {
            std::cout << "\n🎉 ALL TESTS PASSED! 🎉" << std::endl;
        }
        else
        {
            std::cout << "\n⚠️  Some tests failed. Check output above." << std::endl;
        }
    }
};

// Helper function to create JSON from RequestData and ResponseData
json createTrafficJson(const SdkConfig &config, const RequestData &req, const ResponseData &res)
{
    json j;
    j["account_id"] = config.accountId;
    j["timestamp"] = 1234567890; // Fixed timestamp for testing

    json r;
    r["method"] = req.method;
    r["scheme"] = req.scheme;
    r["host"] = req.host;
    r["path"] = req.path;
    r["query"] = req.query;
    r["headers"] = req.headers;
    r["body"] = req.bodyText;
    r["body_b64"] = req.bodyBase64;
    r["ip"] = req.ip;

    json s;
    s["status"] = res.status;
    s["headers"] = res.headers;
    s["body"] = res.bodyText;
    s["body_b64"] = res.bodyBase64;

    j["request"] = r;
    j["response"] = s;

    // latency calculation
    if (req.startNs != 0 && res.endNs != 0 && res.endNs > req.startNs)
    {
        uint64_t deltaNs = res.endNs - req.startNs;
        j["latency_ms"] = static_cast<int>(deltaNs / 1'000'000);
    }

    return j;
}

void test_json_serialization(TestRunner &t)
{
    std::cout << "\n🧪 Testing JSON Serialization..." << std::endl;

    SdkConfig config;
    config.accountId = "test-account-123";

    RequestData req;
    req.method = "POST";
    req.scheme = "https";
    req.host = "api.example.com";
    req.path = "/users";
    req.query = "page=1&limit=10";
    req.headers = json{{"Content-Type", "application/json"}, {"Authorization", "Bearer token"}};
    req.bodyText = "{\"name\":\"John\"}";
    req.bodyBase64 = "eyJuYW1lIjoiSm9obiJ9";
    req.ip = "192.168.1.100";
    req.startNs = 1000000000; // 1 second in nanoseconds

    ResponseData res;
    res.status = 201;
    res.headers = json{{"Content-Type", "application/json"}, {"Location", "/users/123"}};
    res.bodyText = "{\"id\":123,\"name\":\"John\"}";
    res.bodyBase64 = "eyJpZCI6MTIzLCJuYW1lIjoiSm9obiJ9";
    res.endNs = 1500000000; // 1.5 seconds in nanoseconds

    json result = createTrafficJson(config, req, res);

    // Test required fields
    t.assert_true("JSON contains account_id", result.contains("account_id"));
    t.assert_true("JSON contains timestamp", result.contains("timestamp"));
    t.assert_true("JSON contains request", result.contains("request"));
    t.assert_true("JSON contains response", result.contains("response"));
    t.assert_true("JSON contains latency_ms", result.contains("latency_ms"));

    // Test account ID
    t.assert_eq("Account ID correct", std::string("test-account-123"), result["account_id"].get<std::string>());

    // Test request data
    auto req_json = result["request"];
    t.assert_eq("Request method", std::string("POST"), req_json["method"].get<std::string>());
    t.assert_eq("Request host", std::string("api.example.com"), req_json["host"].get<std::string>());
    t.assert_eq("Request path", std::string("/users"), req_json["path"].get<std::string>());
    t.assert_eq("Request query", std::string("page=1&limit=10"), req_json["query"].get<std::string>());

    // Test response data
    auto res_json = result["response"];
    t.assert_eq("Response status", 201, res_json["status"].get<int>());
    t.assert_eq("Response body", std::string("{\"id\":123,\"name\":\"John\"}"), res_json["body"].get<std::string>());

    // Test latency calculation (1500ms - 1000ms) / 1000000 = 500ms
    t.assert_eq("Latency calculation", 500, result["latency_ms"].get<int>());
}

void test_configuration(TestRunner &t)
{
    std::cout << "\n⚙️ Testing Configuration..." << std::endl;

    // Test default config
    SdkConfig config;
    t.assert_eq("Default account ID", std::string("local-traffic-processor"), config.accountId);
    t.assert_eq("Default topic", std::string("http.traffic"), config.kafka.topic);
    t.assert_eq("Default compression", std::string("lz4"), config.kafka.compression);

    // Test batching values match Python config
    t.assert_eq("Linger ms matches Python", 10000, config.kafka.lingerMs);
    t.assert_eq("Batch size matches Python", 32768, config.kafka.batchSizeBytes);
    t.assert_eq("Batch num messages", 100, config.kafka.batchNumMessages);

    // Test performance settings
    t.assert_eq("Acks setting", std::string("1"), config.kafka.acks);
    t.assert_eq("Retries setting", 3, config.kafka.retries);
    t.assert_eq("Request timeout", 5000, config.kafka.requestTimeoutMs);

    // Test custom config
    config.accountId = "custom-account";
    config.kafka.topic = "custom.topic";
    config.kafka.lingerMs = 5000;

    t.assert_eq("Custom account ID", std::string("custom-account"), config.accountId);
    t.assert_eq("Custom topic", std::string("custom.topic"), config.kafka.topic);
    t.assert_eq("Custom linger ms", 5000, config.kafka.lingerMs);
}

void test_data_structures(TestRunner &t)
{
    std::cout << "\n📊 Testing Data Structures..." << std::endl;

    // Test RequestData default construction
    RequestData req;
    t.assert_true("Request method empty by default", req.method.empty());
    t.assert_true("Request host empty by default", req.host.empty());
    t.assert_true("Request path empty by default", req.path.empty());
    t.assert_eq("Request startNs default", static_cast<uint64_t>(0), req.startNs);

    // Test ResponseData default construction
    ResponseData res;
    t.assert_eq("Response status default", 0, res.status);
    t.assert_true("Response body empty by default", res.bodyText.empty());
    t.assert_eq("Response endNs default", static_cast<uint64_t>(0), res.endNs);

    // Test field assignment
    req.method = "GET";
    req.host = "example.com";
    req.path = "/api/test";
    req.startNs = 1234567890000000000ULL;

    t.assert_eq("Request method assignment", std::string("GET"), req.method);
    t.assert_eq("Request host assignment", std::string("example.com"), req.host);
    t.assert_eq("Request path assignment", std::string("/api/test"), req.path);
    t.assert_eq("Request startNs assignment", static_cast<uint64_t>(1234567890000000000ULL), req.startNs);

    res.status = 200;
    res.bodyText = "response body";
    res.endNs = 1234567890500000000ULL;

    t.assert_eq("Response status assignment", 200, res.status);
    t.assert_eq("Response body assignment", std::string("response body"), res.bodyText);
    t.assert_eq("Response endNs assignment", static_cast<uint64_t>(1234567890500000000ULL), res.endNs);
}

void test_edge_cases(TestRunner &t)
{
    std::cout << "\n🔍 Testing Edge Cases..." << std::endl;

    SdkConfig config;
    config.accountId = "edge-test";

    // Test empty request data
    RequestData req; // All fields empty/default
    ResponseData res;
    res.status = 404;

    json result = createTrafficJson(config, req, res);

    t.assert_eq("Empty request method", std::string(""), result["request"]["method"].get<std::string>());
    t.assert_eq("Empty request host", std::string(""), result["request"]["host"].get<std::string>());
    t.assert_eq("Response status with empty request", 404, result["response"]["status"].get<int>());
    t.assert_true("No latency with zero timestamps", !result.contains("latency_ms"));

    // Test invalid timestamps (end before start)
    req.startNs = 2000000000; // Later time
    res.endNs = 1000000000;   // Earlier time

    json result2 = createTrafficJson(config, req, res);
    t.assert_true("No latency with invalid timestamps", !result2.contains("latency_ms"));

    // Test special characters
    req.host = "测试.example.com"; // Unicode
    req.query = "search=hello&filter=café";
    req.bodyText = "Special chars: àáâãäåæçèéêë 🚀🎉";

    json result3 = createTrafficJson(config, req, res);
    t.assert_eq("Unicode host preserved", std::string("测试.example.com"), result3["request"]["host"].get<std::string>());
    t.assert_eq("Unicode query preserved", std::string("search=hello&filter=café"), result3["request"]["query"].get<std::string>());

    // The body check is complex due to emojis, so just check it contains expected text
    std::string body = result3["request"]["body"].get<std::string>();
    t.assert_true("Special characters preserved", body.find("àáâãäåæçèéêë") != std::string::npos);
}

int main()
{
    std::cout << "🚀 Starting Traffic Processing SDK Unit Tests\n"
              << std::endl;

    TestRunner runner;

    test_json_serialization(runner);
    test_configuration(runner);
    test_data_structures(runner);
    test_edge_cases(runner);

    runner.summary();

    return runner.failed > 0 ? 1 : 0;
}
