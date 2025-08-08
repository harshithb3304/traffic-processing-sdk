#pragma once

#include <librdkafka/rdkafka.h>
#include <string>
#include <vector>
#include <memory>
#include <cstdlib>

namespace traffic_processor {

// Kafka configuration with Docker/local detection
struct KafkaConfig {
    std::string bootstrapServers;
    std::string topic{"http.traffic"};
    std::string compression{"lz4"};
    int lingerMs{10};
    int batchKb{512};
    
    KafkaConfig() {
        // Auto-detect if running in Docker
        const char* dockerEnv = std::getenv("DOCKER_ENV");
        if (dockerEnv && std::string(dockerEnv) == "true") {
            bootstrapServers = "kafka:19092";  // Docker internal network
        } else {
            bootstrapServers = "localhost:9092";  // Local development
        }
    }
};

class KafkaProducer {
public:
    explicit KafkaProducer(const KafkaConfig& config);
    ~KafkaProducer();
    void sendBatch(const std::vector<std::string>& jsonRecords);

private:
    KafkaConfig config_;
    rd_kafka_t* producer_;
    rd_kafka_topic_t* topic_;
    
    // Non-copyable
    KafkaProducer(const KafkaProducer&) = delete;
    KafkaProducer& operator=(const KafkaProducer&) = delete;
};

} // namespace traffic_processor