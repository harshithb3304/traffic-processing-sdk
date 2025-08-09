#include "traffic_processor/sdk.hpp"

#include <chrono>
#include <iostream>
#include <nlohmann/json.hpp>

using namespace traffic_processor;

TrafficProcessorSdk &TrafficProcessorSdk::instance()
{
    static TrafficProcessorSdk sdk;
    return sdk;
}

void TrafficProcessorSdk::initialize()
{
    // Simple local configuration - no environment variables needed
    SdkConfig config; // Uses all default values
    initialize(config);
}

void TrafficProcessorSdk::initialize(const SdkConfig &config)
{
    cfg_ = config;
    producer_ = std::make_unique<KafkaProducer>(cfg_.kafka);
    // No worker thread needed - sending directly to Kafka

    // Start a simple polling thread for rdkafka housekeeping
    stop_ = false;
    worker_ = std::thread(&TrafficProcessorSdk::pollingLoop, this);
}

TrafficProcessorSdk::~TrafficProcessorSdk()
{
    shutdown();
}

void TrafficProcessorSdk::shutdown()
{
    bool expected = false;
    if (stop_.compare_exchange_strong(expected, true))
    {
        if (worker_.joinable())
            worker_.join();
    }
}

void TrafficProcessorSdk::printKafkaStats()
{
    if (producer_)
    {
        producer_->printStats();
    }
    else
    {
        std::cout << "SDK not initialized" << std::endl;
    }
}

void TrafficProcessorSdk::capture(const RequestData &req, const ResponseData &res)
{
    using nlohmann::json;
    json j;
    j["account_id"] = cfg_.accountId;
    j["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
                         std::chrono::system_clock::now().time_since_epoch())
                         .count();

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

    // latency
    if (req.startNs != 0 && res.endNs != 0 && res.endNs > req.startNs)
    {
        uint64_t deltaNs = res.endNs - req.startNs;
        j["latency_ms"] = static_cast<int>(deltaNs / 1'000'000);
    }

    std::string serialized = j.dump();

    // Send directly to Kafka - no queue needed! rdkafka has its own internal queue
    if (producer_)
    {
        producer_->send(serialized);
    }
}

void TrafficProcessorSdk::pollingLoop()
{
    // Simple polling loop for rdkafka housekeeping only
    while (!stop_)
    {
        if (producer_)
        {
            producer_->poll(100); // Poll for delivery reports and statistics
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Final flush on shutdown
    if (producer_)
    {
        producer_->flush(2000);
    }
}