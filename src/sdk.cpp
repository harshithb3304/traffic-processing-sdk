#include "traffic_processor/sdk.hpp"

#include <chrono>
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
    cfg_ = config;
    producer_ = std::make_unique<KafkaProducer>(cfg_.kafka);
    stop_ = false;
    worker_ = std::thread(&TrafficProcessorSdk::workerLoop, this);
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
        cv_.notify_all();
        if (worker_.joinable())
            worker_.join();
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
    {
        std::lock_guard<std::mutex> lk(mtx_);
        queue_.push(std::move(serialized));
    }
    cv_.notify_one();
}

void TrafficProcessorSdk::workerLoop()
{
    std::unique_lock<std::mutex> lk(mtx_);
    while (!stop_)
    {
        if (queue_.empty())
        {
            cv_.wait_for(lk, std::chrono::milliseconds(100)); // Short wait for new messages
        }

        while (!queue_.empty())
        {
            std::string next;
            next = std::move(queue_.front());
            queue_.pop();
            lk.unlock();
            try
            {
                producer_->send(next);
            }
            catch (...)
            {
            }
            lk.lock();
        }
    }

    // Drain any remaining messages after stop is signaled
    while (!queue_.empty())
    {
        std::string next = std::move(queue_.front());
        queue_.pop();
        lk.unlock();
        try
        {
            producer_->send(next);
        }
        catch (...)
        {
        }
        lk.lock();
    }
}