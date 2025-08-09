#pragma once

#include <librdkafka/rdkafka.h>
#include <string>
#include <vector>
#include <memory>
#include <cstdlib>

namespace traffic_processor
{

    // Kafka configuration with Docker/local detection and comprehensive batching
    struct KafkaConfig
    {
        std::string bootstrapServers;
        std::string topic{"http.traffic"};
        std::string compression{"lz4"};

        // Batching Configuration (based on Python code that works)
        int lingerMs{10000};       // Wait up to 10 seconds before sending batch (like Python code)
        int batchSizeBytes{32768}; // 32KB batch size (like Python batch.size)
        int batchNumMessages{100}; // Max 100 messages per batch
        int queueBufferingMaxMessages{10000};
        int queueBufferingMaxKbytes{32768};

        // Performance & Reliability
        std::string acks{"1"};      // Wait for leader acknowledgment
        int retries{3};             // Retry failed sends 3 times
        int requestTimeoutMs{5000}; // 5s timeout for requests

        KafkaConfig()
        {
            // Auto-detect if running in Docker
            const char *dockerEnv = std::getenv("DOCKER_ENV");
            if (dockerEnv && std::string(dockerEnv) == "true")
            {
                bootstrapServers = "kafka:19092"; // Docker internal network
            }
            else
            {
                bootstrapServers = "localhost:9092"; // Local development
            }
        }

        // Simplified configuration - no profiles needed
    };

    class KafkaProducer
    {
    public:
        explicit KafkaProducer(const KafkaConfig &config);
        ~KafkaProducer();

        // Send a JSON record to Kafka (rdkafka auto-batching)
        void send(const std::string &jsonRecord);

        // Poll for delivery reports
        void poll(int timeoutMs = 0);

        // Force immediate flush of all pending messages
        void flush(int timeoutMs = 1000);

        // Get current queue statistics
        void printStats() const;

    private:
        KafkaConfig config_;
        rd_kafka_t *producer_;
        rd_kafka_topic_t *topic_;

        // Non-copyable
        KafkaProducer(const KafkaProducer &) = delete;
        KafkaProducer &operator=(const KafkaProducer &) = delete;
    };

}