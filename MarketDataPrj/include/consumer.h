// src/include/consumer.h
#pragma once

#include <string>
#include <atomic>

#include <rdkafka.h>          // ✅ OBLIGATOIRE

#include "marketdata.h"
#include "tick_queue.h"

class Consumer {
public:
    Consumer(const std::string& brokers,
        const std::string& topic,
        TickQueue& queue);

    ~Consumer();

    // À lancer dans un std::thread
    void run();
    void stop();

    Consumer(const Consumer&) = delete;
    Consumer& operator=(const Consumer&) = delete;

private:
    MarketDataTick parseMessage(const std::string& msg);

private:
    std::atomic<bool> running_{ true };
    TickQueue& queue_;

    // === Kafka ===
    rd_kafka_t* rk_{ nullptr };
    rd_kafka_conf_t* conf_{ nullptr };
    rd_kafka_topic_partition_list_t* topics_{ nullptr };
};
