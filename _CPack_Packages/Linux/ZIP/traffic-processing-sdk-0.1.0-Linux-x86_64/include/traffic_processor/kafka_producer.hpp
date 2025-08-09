#pragma once

#include <librdkafka/rdkafka.h>
#include <string>
#include <memory>
#include <map>
#include <cstdlib>

namespace traffic_processor
{

    struct KafkaConfig
    {
        std::string bootstrapServers;
        std::string topic{"http.traffic"};
        std::string compression{"lz4"};

        // Batching configuration
        int lingerMs{10000};
        int batchSizeBytes{32768};
        int batchNumMessages{100};
        int queueBufferingMaxMessages{10000};
        int queueBufferingMaxKbytes{32768};

        // Performance & reliability
        std::string acks{"1"};
        int retries{3};
        int requestTimeoutMs{5000};

        // Optional: arbitrary librdkafka properties passed as a map/object.
        // Any keys provided here override the typed fields or add new ones.
        // Example usage (object-style):
        //   cfg.kafka.extraProperties["enable.idempotence"] = "true";
        std::map<std::string, std::string> extraProperties;

        KafkaConfig()
        {
            // Auto-detect if running in Docker
            const char *dockerEnv = std::getenv("DOCKER_ENV");
            if (dockerEnv && std::string(dockerEnv) == "true")
            {
                bootstrapServers = "kafka:19092";
            }
            else
            {
                bootstrapServers = "localhost:9092";
            }

            // Environment overrides (provide any subset; rest use defaults)
            if (const char *url = std::getenv("KAFKA_URL"))
            {
                bootstrapServers = url;
            }
            if (const char *t = std::getenv("KAFKA_TOPIC"))
            {
                topic = t;
            }
            if (const char *c = std::getenv("KAFKA_COMPRESSION"))
            {
                compression = c;
            }
            if (const char *a = std::getenv("KAFKA_ACKS"))
            {
                acks = a;
            }
            if (const char *linger = std::getenv("KAFKA_BATCH_TIMEOUT"))
            {
                try
                {
                    lingerMs = std::stoi(linger);
                }
                catch (...)
                {
                }
            }
            if (const char *bnm = std::getenv("KAFKA_BATCH_SIZE"))
            {
                try
                {
                    batchNumMessages = std::stoi(bnm);
                }
                catch (...)
                {
                }
            }
            if (const char *bs = std::getenv("KAFKA_BATCH_SIZE_BYTES"))
            {
                try
                {
                    batchSizeBytes = std::stoi(bs);
                }
                catch (...)
                {
                }
            }
            if (const char *rq = std::getenv("KAFKA_REQUEST_TIMEOUT_MS"))
            {
                try
                {
                    requestTimeoutMs = std::stoi(rq);
                }
                catch (...)
                {
                }
            }
            if (const char *rb = std::getenv("KAFKA_BUFFER_MAX_MESSAGES"))
            {
                try
                {
                    queueBufferingMaxMessages = std::stoi(rb);
                }
                catch (...)
                {
                }
            }
            if (const char *rk = std::getenv("KAFKA_BUFFER_MAX_KBYTES"))
            {
                try
                {
                    queueBufferingMaxKbytes = std::stoi(rk);
                }
                catch (...)
                {
                }
            }
        }
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

        KafkaProducer(const KafkaProducer &) = delete;
        KafkaProducer &operator=(const KafkaProducer &) = delete;
    };

}