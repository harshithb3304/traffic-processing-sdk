#include "traffic_processor/kafka_producer.hpp"
#include <iostream>
#include <cstring>
#include <chrono>
#include <iomanip>

using namespace traffic_processor;

// Delivery report callback with batch tracking
static void delivery_report_callback(rd_kafka_t *rk, const rd_kafka_message_t *rkmessage, void *opaque)
{
    static int total_delivered = 0;
    static auto last_batch_time = std::chrono::steady_clock::now();
    static int batch_message_count = 0;

    if (rkmessage->err)
    {
        std::cerr << "KAFKA ERROR: Message delivery failed - " << rd_kafka_err2str(rkmessage->err) << std::endl;
    }
    else
    {
        total_delivered++;
        batch_message_count++;

        auto now = std::chrono::steady_clock::now();
        auto time_since_last = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_batch_time).count();

        // Report batch details when enough time passes or enough messages accumulate
        if (time_since_last >= 500 || batch_message_count >= 25)
        {
            std::cout << "KAFKA BATCH DELIVERED: " << batch_message_count
                      << " messages in " << time_since_last
                      << "ms (Total delivered: " << total_delivered << ")" << std::endl;

            batch_message_count = 0;
            last_batch_time = now;
        }
    }
}

KafkaProducer::KafkaProducer(const KafkaConfig &config) : config_(config), producer_(nullptr), topic_(nullptr)
{
    std::cout << "Initializing Kafka Producer with Enhanced Batching..." << std::endl;
    std::cout << "   Bootstrap Servers: " << config_.bootstrapServers << std::endl;
    std::cout << "   Topic: " << config_.topic << std::endl;
    std::cout << "   Batch Config: " << config_.batchNumMessages << " msgs, "
              << config_.batchSizeBytes / 1024 << "KB, " << config_.lingerMs << "ms linger" << std::endl;
    std::cout << "   Watch for 'Message #X queued' and 'BATCH DELIVERED' logs!" << std::endl;

    // Create Kafka configuration
    rd_kafka_conf_t *conf = rd_kafka_conf_new();
    char errstr[512];

    // Set configuration properties
    if (rd_kafka_conf_set(conf, "bootstrap.servers", config_.bootstrapServers.c_str(), errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK)
    {
        std::cerr << "Failed to set bootstrap.servers: " << errstr << std::endl;
        rd_kafka_conf_destroy(conf);
        throw std::runtime_error("Failed to configure Kafka producer");
    }

    // Basic Kafka settings (matching Python code)
    rd_kafka_conf_set(conf, "compression.type", "zstd", errstr, sizeof(errstr)); // Like Python
    rd_kafka_conf_set(conf, "acks", "0", errstr, sizeof(errstr));                // Like Python (fastest)
    rd_kafka_conf_set(conf, "retries", std::to_string(config_.retries).c_str(), errstr, sizeof(errstr));
    rd_kafka_conf_set(conf, "request.timeout.ms", std::to_string(config_.requestTimeoutMs).c_str(), errstr, sizeof(errstr));

    // CRITICAL BATCHING SETTINGS (matching Python code)
    rd_kafka_conf_set(conf, "linger.ms", std::to_string(config_.lingerMs).c_str(), errstr, sizeof(errstr));                  // 10000ms like Python
    rd_kafka_conf_set(conf, "batch.num.messages", std::to_string(config_.batchNumMessages).c_str(), errstr, sizeof(errstr)); // 100 like Python
    rd_kafka_conf_set(conf, "batch.size", std::to_string(config_.batchSizeBytes).c_str(), errstr, sizeof(errstr));           // 32KB like Python

    // Set delivery report callback for tracking
    rd_kafka_conf_set_dr_msg_cb(conf, delivery_report_callback);

    // Create producer instance
    producer_ = rd_kafka_new(RD_KAFKA_PRODUCER, conf, errstr, sizeof(errstr));
    if (!producer_)
    {
        std::cerr << "Failed to create producer: " << errstr << std::endl;
        throw std::runtime_error("Failed to create Kafka producer");
    }

    // Create topic handle
    topic_ = rd_kafka_topic_new(producer_, config_.topic.c_str(), nullptr);
    if (!topic_)
    {
        std::cerr << "Failed to create topic: " << config_.topic << std::endl;
        rd_kafka_destroy(producer_);
        throw std::runtime_error("Failed to create Kafka topic");
    }

    std::cout << "Kafka Producer initialized successfully" << std::endl;
}

KafkaProducer::~KafkaProducer()
{
    if (producer_)
    {
        // Flush any pending messages before shutdown
        std::cout << "Shutting down Kafka producer, flushing pending messages..." << std::endl;
        flush(2000); // Give 2 seconds for final flush
    }

    if (topic_)
    {
        rd_kafka_topic_destroy(topic_);
        topic_ = nullptr;
    }
    if (producer_)
    {
        rd_kafka_destroy(producer_);
        producer_ = nullptr;
    }
}

void KafkaProducer::send(const std::string &jsonRecord)
{
    if (!producer_ || !topic_)
    {
        std::cerr << "Kafka producer not initialized" << std::endl;
        return;
    }

    static int message_count = 0;
    message_count++;

    int result = rd_kafka_produce(
        topic_,
        RD_KAFKA_PARTITION_UA, // automatic partitioning
        RD_KAFKA_MSG_F_COPY,
        const_cast<char *>(jsonRecord.c_str()),
        jsonRecord.length(),
        nullptr, 0,
        nullptr);

    if (result == -1)
    {
        std::cerr << "Failed to produce message #" << message_count << ": " << rd_kafka_err2str(rd_kafka_last_error()) << std::endl;
    }
    else if (message_count % 25 == 0)
    {
        int queue_len = rd_kafka_outq_len(producer_);
        std::cout << "KAFKA QUEUE: Message " << message_count
                  << " queued, queue depth: " << queue_len
                  << ", batch config: " << config_.batchNumMessages << " msgs/"
                  << config_.lingerMs << "ms" << std::endl;
    }

    // Let rdkafka handle batching automatically based on configuration
}

void KafkaProducer::poll(int timeoutMs)
{
    if (!producer_)
    {
        std::cerr << "Kafka producer not initialized" << std::endl;
        return;
    }

    // Poll for delivery reports and internal housekeeping
    rd_kafka_poll(producer_, timeoutMs);
}

void KafkaProducer::flush(int timeoutMs)
{
    if (!producer_)
    {
        std::cerr << "Kafka producer not initialized" << std::endl;
        return;
    }

    std::cout << "Flushing rdkafka queue..." << std::endl;
    int remaining = rd_kafka_flush(producer_, timeoutMs);
    if (remaining > 0)
    {
        std::cerr << remaining << " messages still in queue after flush timeout" << std::endl;
    }
    else
    {
        std::cout << "All messages flushed successfully" << std::endl;
    }
}

void KafkaProducer::printStats() const
{
    if (!producer_)
    {
        std::cerr << "Kafka producer not initialized" << std::endl;
        return;
    }

    // Print basic queue statistics
    int outq_len = rd_kafka_outq_len(producer_);
    std::cout << "=== Kafka Producer Statistics ===" << std::endl;
    std::cout << "Messages in outbound queue: " << outq_len << std::endl;
    std::cout << "Topic: " << config_.topic << std::endl;
    std::cout << "Batch config: " << config_.batchNumMessages << " msgs, "
              << config_.batchSizeBytes / 1024 << "KB, " << config_.lingerMs << "ms" << std::endl;
    std::cout << "=================================" << std::endl;
}