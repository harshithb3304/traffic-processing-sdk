#include <crow.h>
#include <nlohmann/json.hpp>

#include "traffic_processor/sdk.hpp"
#include "traffic_processor/crow_middleware.hpp"

#include <chrono>
#include <iostream>
#include <cstdlib>

using namespace traffic_processor;

static SdkConfig buildConfigFromEnv()
{
    SdkConfig cfg;

    if (const char *url = std::getenv("KAFKA_URL"))
    {
        cfg.kafka.bootstrapServers = url;
    }
    if (const char *topic = std::getenv("KAFKA_TOPIC"))
    {
        cfg.kafka.topic = topic;
    }
    if (const char *comp = std::getenv("KAFKA_COMPRESSION"))
    {
        cfg.kafka.compression = comp;
    }
    if (const char *acks = std::getenv("KAFKA_ACKS"))
    {
        cfg.kafka.acks = acks;
    }

    if (const char *linger = std::getenv("KAFKA_BATCH_TIMEOUT"))
    {
        try
        {
            cfg.kafka.lingerMs = std::stoi(linger);
        }
        catch (...)
        {
        }
    }
    if (const char *bnm = std::getenv("KAFKA_BATCH_SIZE"))
    {
        try
        {
            cfg.kafka.batchNumMessages = std::stoi(bnm);
        }
        catch (...)
        {
        }
    }
    if (const char *bs = std::getenv("KAFKA_BATCH_SIZE_BYTES"))
    {
        try
        {
            cfg.kafka.batchSizeBytes = std::stoi(bs);
        }
        catch (...)
        {
        }
    }
    if (const char *rq = std::getenv("KAFKA_REQUEST_TIMEOUT_MS"))
    {
        try
        {
            cfg.kafka.requestTimeoutMs = std::stoi(rq);
        }
        catch (...)
        {
        }
    }
    if (const char *rb = std::getenv("KAFKA_BUFFER_MAX_MESSAGES"))
    {
        try
        {
            cfg.kafka.queueBufferingMaxMessages = std::stoi(rb);
        }
        catch (...)
        {
        }
    }
    if (const char *rk = std::getenv("KAFKA_BUFFER_MAX_KBYTES"))
    {
        try
        {
            cfg.kafka.queueBufferingMaxKbytes = std::stoi(rk);
        }
        catch (...)
        {
        }
    }

    return cfg;
}

// removed: local maybe_base64; provided by reusable middleware header

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    std::cout << "Starting Traffic Processor SDK Demo Server..." << std::endl;
    // Build a single object with all parameters (object-based config)
    SdkConfig cfg = buildConfigFromEnv();
    TrafficProcessorSdk::instance().initialize(cfg);
    std::cout << "SDK initialized successfully" << std::endl;

    traffic_processor::crow_integration::TrafficApp app_with_middleware;

    // Main echo route - supports GET and POST only
    CROW_ROUTE(app_with_middleware, "/echo").methods(crow::HTTPMethod::GET, crow::HTTPMethod::POST)([](const crow::request &req)
                                                                                                    {
        crow::response resp;
        nlohmann::json j;
        j["method"] = crow::method_name(req.method);
        j["body"] = req.body;
        j["url"] = req.url;
        resp.set_header("content-type", "application/json");
        resp.code = 200;
        resp.body = j.dump();
        return resp; });

    // Catch-all route for unsupported methods on /echo
    CROW_ROUTE(app_with_middleware, "/echo").methods(crow::HTTPMethod::PUT, crow::HTTPMethod::DELETE, crow::HTTPMethod::PATCH, crow::HTTPMethod::HEAD, crow::HTTPMethod::OPTIONS)([](const crow::request &req)
                                                                                                                                                                                  {
        crow::response resp;
        resp.code = 405;
        resp.set_header("content-type", "application/json");
        resp.body = "{\"error\":\"Method Not Allowed\",\"message\":\"Only GET and POST are supported on /echo\"}";
        return resp; });

    // Catch-all route for any other path (404 errors)
    CROW_ROUTE(app_with_middleware, "/<path>")([](const crow::request &req, const std::string &path)
                                               {
        crow::response resp;
        resp.code = 404;
        resp.set_header("content-type", "application/json");
        resp.body = "{\"error\":\"Not Found\",\"path\":\"/" + path + "\",\"message\":\"Endpoint not found\"}";
        return resp; });

    std::cout << "Server starting on http://0.0.0.0:8080" << std::endl;
    std::cout << "Supports: GET, POST on /echo endpoint only" << std::endl;
    std::cout << "Try: curl -X POST http://localhost:8080/echo -d '{\"test\":\"data\"}' -H 'Content-Type: application/json'" << std::endl;
    std::cout << "Note: All requests (including errors) are logged to Kafka" << std::endl;

    app_with_middleware.port(8080).multithreaded().run();

    std::cout << "Shutting down..." << std::endl;
    TrafficProcessorSdk::instance().shutdown();
    return 0;
}