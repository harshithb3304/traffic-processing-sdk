#include <crow.h>
#include <nlohmann/json.hpp>

#include "traffic_processor/sdk.hpp"

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

static std::string maybe_base64(const std::string &body)
{
    return crow::utility::base64encode(body, body.size());
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    std::cout << "Starting Traffic Processor SDK Demo Server..." << std::endl;
    // Build a single object with all parameters (object-based config)
    SdkConfig cfg = buildConfigFromEnv();
    TrafficProcessorSdk::instance().initialize(cfg);
    std::cout << "SDK initialized successfully" << std::endl;

    crow::SimpleApp app;

    // Middleware to capture ALL requests (including errors)
    struct TrafficMiddleware
    {
        struct context
        {
            std::chrono::steady_clock::duration start_time;
        };

        void before_handle(crow::request &req, crow::response &res, context &ctx)
        {
            // Store start time for this request
            ctx.start_time = std::chrono::steady_clock::now().time_since_epoch();
        }

        void after_handle(crow::request &req, crow::response &res, context &ctx)
        {
            auto start = ctx.start_time;
            uint64_t startNs = std::chrono::duration_cast<std::chrono::nanoseconds>(start).count();

            // Build capture data for ALL requests
            RequestData r;
            r.method = crow::method_name(req.method);
            r.scheme = req.get_header_value("X-Forwarded-Proto");
            if (r.scheme.empty())
                r.scheme = "http";
            r.host = req.get_header_value("Host");
            r.path = req.url;
            r.query = "";
            nlohmann::json hreq = nlohmann::json::object();
            for (const auto &[k, v] : req.headers)
                hreq[k] = v;
            r.headers = hreq;
            r.bodyText = req.body;
            r.bodyBase64 = maybe_base64(req.body);
            r.ip = req.remote_ip_address;
            r.startNs = startNs;

            ResponseData s;
            s.status = res.code;
            nlohmann::json hres = nlohmann::json::object();
            for (const auto &[k, v] : res.headers)
                hres[k] = v;
            s.headers = hres;
            s.bodyText = res.body;
            s.bodyBase64 = crow::utility::base64encode(res.body, res.body.size());
            auto end = std::chrono::steady_clock::now().time_since_epoch();
            s.endNs = std::chrono::duration_cast<std::chrono::nanoseconds>(end).count();

            std::cout << "HTTP Request: " << r.method << " " << r.path << " -> " << s.status << " from " << r.ip << std::endl;
            TrafficProcessorSdk::instance().capture(r, s);
            std::cout << "Traffic captured and queued for Kafka" << std::endl;
        }
    };

    crow::App<TrafficMiddleware> app_with_middleware;

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