#include "traffic_processor/kafka_producer.hpp"
#include <iostream>
#include <cstring>

using namespace traffic_processor;

KafkaProducer::KafkaProducer(const KafkaConfig& config) : config_(config), producer_(nullptr), topic_(nullptr) {
    std::cout << "Initializing Kafka Producer..." << std::endl;
    std::cout << "   Bootstrap Servers: " << config_.bootstrapServers << std::endl;
    std::cout << "   Topic: " << config_.topic << std::endl;
    
    // Create Kafka configuration
    rd_kafka_conf_t* conf = rd_kafka_conf_new();
    char errstr[512];
    
    // Set configuration properties
    if (rd_kafka_conf_set(conf, "bootstrap.servers", config_.bootstrapServers.c_str(), errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
        std::cerr << "Failed to set bootstrap.servers: " << errstr << std::endl;
        rd_kafka_conf_destroy(conf);
        throw std::runtime_error("Failed to configure Kafka producer");
    }
    
    // Set other properties (ignore errors for optional settings)
    rd_kafka_conf_set(conf, "compression.type", config_.compression.c_str(), errstr, sizeof(errstr));
    rd_kafka_conf_set(conf, "linger.ms", std::to_string(config_.lingerMs).c_str(), errstr, sizeof(errstr));
    rd_kafka_conf_set(conf, "acks", "1", errstr, sizeof(errstr));
    
    // Create producer instance
    producer_ = rd_kafka_new(RD_KAFKA_PRODUCER, conf, errstr, sizeof(errstr));
    if (!producer_) {
        std::cerr << "Failed to create producer: " << errstr << std::endl;
        throw std::runtime_error("Failed to create Kafka producer");
    }
    
    // Create topic handle
    topic_ = rd_kafka_topic_new(producer_, config_.topic.c_str(), nullptr);
    if (!topic_) {
        std::cerr << "Failed to create topic: " << config_.topic << std::endl;
        rd_kafka_destroy(producer_);
        throw std::runtime_error("Failed to create Kafka topic");
    }
    
    std::cout << "Kafka Producer initialized successfully" << std::endl;
}

KafkaProducer::~KafkaProducer() {
    if (topic_) {
        rd_kafka_topic_destroy(topic_);
        topic_ = nullptr;
    }
    if (producer_) {
        // Flush any pending messages
        rd_kafka_flush(producer_, 1000);
        rd_kafka_destroy(producer_);
        producer_ = nullptr;
    }
}

void KafkaProducer::sendBatch(const std::vector<std::string>& jsonRecords) {
    if (!producer_ || !topic_) {
        std::cerr << "Kafka producer not initialized" << std::endl;
        return;
    }
    
    std::cout << "Sending batch of " << jsonRecords.size() << " messages to Kafka topic: " << config_.topic << std::endl;
    
    for (const auto& record : jsonRecords) {
        // Produce message (non-blocking)
        int result = rd_kafka_produce(
            topic_,
            RD_KAFKA_PARTITION_UA,  // Use automatic partitioning
            RD_KAFKA_MSG_F_COPY,    // Copy the payload
            const_cast<char*>(record.c_str()),
            record.length(),
            nullptr, 0,  // No key
            nullptr      // No per-message opaque
        );
        
        if (result == -1) {
            std::cerr << "Failed to produce message: " << rd_kafka_err2str(rd_kafka_last_error()) << std::endl;
        } else {
            std::cout << "Message queued: " << record.substr(0, 80) << "..." << std::endl;
        }
        
        // Poll to handle delivery reports
        rd_kafka_poll(producer_, 0);
    }
    
    // Flush to ensure messages are sent
    int remaining = rd_kafka_flush(producer_, 1000);  // 1 second timeout
    if (remaining > 0) {
        std::cerr << remaining << " messages still in queue after flush" << std::endl;
    } else {
        std::cout << "Batch sent successfully" << std::endl;
    }
}