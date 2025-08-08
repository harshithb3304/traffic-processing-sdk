#include "traffic_processor/kafka_producer.hpp"

using namespace traffic_processor;

KafkaProducer::KafkaProducer(const KafkaConfig& config) : config_(config) {
    // Simple local Kafka configuration - no security needed
    cppkafka::Configuration conf = {
        {"bootstrap.servers", config_.bootstrapServers},
        {"compression.type", config_.compression},
        {"linger.ms", std::to_string(config_.lingerMs)},
        {"queue.buffering.max.kbytes", std::to_string(config_.batchKb)},
        {"acks", "1"}
    };

    producer_ = std::make_unique<cppkafka::Producer>(conf);
    topic_ = config_.topic;
}

void KafkaProducer::sendBatch(const std::vector<std::string>& jsonRecords) {
    for (const auto& rec : jsonRecords) {
        producer_->produce(cppkafka::MessageBuilder(topic_).payload(rec));
    }
    producer_->flush();
}