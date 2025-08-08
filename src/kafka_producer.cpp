#include "traffic_processor/kafka_producer.hpp"
#include <iostream>

using namespace traffic_processor;

KafkaProducer::KafkaProducer(const KafkaConfig& config) : config_(config) {
    std::cout << "🚀 Initializing Kafka Producer..." << std::endl;
    std::cout << "   Bootstrap Servers: " << config_.bootstrapServers << std::endl;
    std::cout << "   Topic: " << config_.topic << std::endl;
    
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
    std::cout << "✅ Kafka Producer initialized successfully" << std::endl;
}

void KafkaProducer::sendBatch(const std::vector<std::string>& jsonRecords) {
    std::cout << "📦 Sending batch of " << jsonRecords.size() << " messages to Kafka topic: " << topic_ << std::endl;
    
    for (const auto& rec : jsonRecords) {
        producer_->produce(cppkafka::MessageBuilder(topic_).payload(rec));
        std::cout << "📤 Message: " << rec << std::endl;
    }
    producer_->flush();
    std::cout << "✅ Batch sent successfully!" << std::endl;
}