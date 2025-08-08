#pragma once

#include <cppkafka/cppkafka.h>
#include <string>
#include <vector>
#include <memory>

namespace traffic_processor {

// Simplified local-only Kafka configuration
struct KafkaConfig {
    std::string bootstrapServers{"localhost:9092"};
    std::string topic{"http.traffic"};
    std::string compression{"lz4"};
    int lingerMs{10};
    int batchKb{512};
};

class KafkaProducer {
public:
    explicit KafkaProducer(const KafkaConfig& config);
    void sendBatch(const std::vector<std::string>& jsonRecords);

private:
    KafkaConfig config_;
    std::unique_ptr<cppkafka::Producer> producer_;
    std::string topic_;
};

} // namespace traffic_processor