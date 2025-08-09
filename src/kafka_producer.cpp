#include "traffic_processor/kafka_producer.hpp"
#include <iostream>
#include <cstring>

using namespace traffic_processor;

static void delivery_report_callback(rd_kafka_t *rk, const rd_kafka_message_t *rkmessage, void * /*opaque*/)
{
    if (rkmessage->err)
    {
        std::cerr << "KAFKA ERROR: Message delivery failed - " << rd_kafka_err2str(rkmessage->err) << std::endl;
    }
}

KafkaProducer::KafkaProducer(const KafkaConfig &config) : config_(config), producer_(nullptr), topic_(nullptr)
{

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

    // Basic Kafka settings (configurable)
    rd_kafka_conf_set(conf, "compression.type", config_.compression.c_str(), errstr, sizeof(errstr));
    rd_kafka_conf_set(conf, "acks", config_.acks.c_str(), errstr, sizeof(errstr));
    rd_kafka_conf_set(conf, "retries", std::to_string(config_.retries).c_str(), errstr, sizeof(errstr));
    rd_kafka_conf_set(conf, "request.timeout.ms", std::to_string(config_.requestTimeoutMs).c_str(), errstr, sizeof(errstr));

    // Batching settings
    rd_kafka_conf_set(conf, "linger.ms", std::to_string(config_.lingerMs).c_str(), errstr, sizeof(errstr));
    rd_kafka_conf_set(conf, "batch.num.messages", std::to_string(config_.batchNumMessages).c_str(), errstr, sizeof(errstr));
    rd_kafka_conf_set(conf, "batch.size", std::to_string(config_.batchSizeBytes).c_str(), errstr, sizeof(errstr));
    rd_kafka_conf_set(conf, "queue.buffering.max.messages", std::to_string(config_.queueBufferingMaxMessages).c_str(), errstr, sizeof(errstr));
    rd_kafka_conf_set(conf, "queue.buffering.max.kbytes", std::to_string(config_.queueBufferingMaxKbytes).c_str(), errstr, sizeof(errstr));

    // Apply arbitrary user-provided properties last so they override defaults
    for (const auto &kv : config_.extraProperties)
    {
        rd_kafka_conf_set(conf, kv.first.c_str(), kv.second.c_str(), errstr, sizeof(errstr));
    }

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

    // Kafka Producer initialized
}

KafkaProducer::~KafkaProducer()
{
    if (producer_)
    {
        // Flush pending messages before shutdown
        flush(2000);
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
        std::cerr << "Failed to produce message: " << rd_kafka_err2str(rd_kafka_last_error()) << std::endl;
    }
    // Drive delivery reports and internal callbacks without blocking
    rd_kafka_poll(producer_, 0);
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

    int remaining = rd_kafka_flush(producer_, timeoutMs);
    if (remaining > 0)
    {
        std::cerr << remaining << " messages still in queue after flush timeout" << std::endl;
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