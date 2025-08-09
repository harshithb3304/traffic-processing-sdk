#pragma once

#include <string>
#include <vector>

#include <nlohmann/json.hpp>
#include "traffic_processor/kafka_producer.hpp"

namespace traffic_processor
{

    struct SdkConfig
    {
        std::string accountId{"local-traffic-processor"};
        KafkaConfig kafka; // Uses default localhost:9092
    };

    struct RequestData
    {
        std::string method;
        std::string scheme;
        std::string host;
        std::string path;
        std::string query;
        nlohmann::json headers;
        std::string bodyText;
        std::string bodyBase64;
        std::string ip;
        uint64_t startNs{0};
    };

    struct ResponseData
    {
        int status{0};
        nlohmann::json headers;
        std::string bodyText;
        std::string bodyBase64;
        uint64_t endNs{0};
    };

    class TrafficProcessorSdk
    {
    public:
        static TrafficProcessorSdk &instance();
        void initialize();                        // Simple initialization with defaults
        void initialize(const SdkConfig &config); // Initialize with custom config
        void capture(const RequestData &req, const ResponseData &res);
        void shutdown();
        void printKafkaStats(); // Print current Kafka producer statistics

    private:
        TrafficProcessorSdk() = default;
        ~TrafficProcessorSdk();
        TrafficProcessorSdk(const TrafficProcessorSdk &) = delete;
        TrafficProcessorSdk &operator=(const TrafficProcessorSdk &) = delete;

        SdkConfig cfg_{};
        std::unique_ptr<KafkaProducer> producer_;
    };

} // namespace traffic_processor
