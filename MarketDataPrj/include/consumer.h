// src/include/consumer.h
#pragma once

#include <string>
#include <atomic>
//#include <rdkafka.h>
#include "marketdata.h"
#include "tick_queue.h"

class Consumer {
public:
    Consumer(const std::string& brokers,
        const std::string& topic,
        TickQueue& queue);
    ~Consumer();

    void run();      // à lancer dans un std::thread
    void stop();

    Consumer(const Consumer&) = delete;
    Consumer& operator=(const Consumer&) = delete;

private:
    MarketDataTick parseMessage(const std::string& msg);

    std::atomic<bool> running_{ true };
    TickQueue& queue_;

   // rd_kafka_t* rk_ = nullptr;
   // rd_kafka_conf_t* conf_ = nullptr;
   // rd_kafka_topic_partition_list_t* topics_ = nullptr;
};
