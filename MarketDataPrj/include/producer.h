#pragma once

#include <string>
#include <thread>
#include <atomic>

#include <rdkafka.h>
#include "marketdata.h"

class Producer {
public:
    Producer(const std::string& brokers,
        const std::string& topic,
        const std::string& twelveApiKey);

    ~Producer();

    // Envoi Kafka
    void sendRaw(const std::string& msg);
    void sendTick(const MarketDataTick& tick);

    // Twelve Data
    void startTwelveData(const std::string& symbol, int intervalSec = 60);
    void stop();

    Producer(const Producer&) = delete;
    Producer& operator=(const Producer&) = delete;

private:
    void twelveDataLoop(const std::string& symbol, int intervalSec);

private:
    // === CONFIG ===
    std::string brokers_;
    std::string topicName_;
    std::string twelveApiKey_;

    // === THREAD ===
    std::thread worker_;
    std::atomic<bool> running_{ false };

    // === KAFKA ===
    rd_kafka_t* rk_ = nullptr;
    rd_kafka_conf_t* conf_ = nullptr;
    rd_kafka_topic_t* topic_ = nullptr;
};
