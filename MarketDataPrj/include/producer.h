// src/include/producer.h
#pragma once

#include <string>
//#include <rdkafka.h>
#include "marketdata.h"

class Producer {
public:
    Producer(const std::string& brokers, const std::string& topic);
    ~Producer();

    void sendRaw(const std::string& msg);
    void sendTick(const MarketDataTick& tick);

    Producer(const Producer&) = delete;
    Producer& operator=(const Producer&) = delete;

private:
    //rd_kafka_t* rk_ = nullptr;
    //rd_kafka_conf_t* conf_ = nullptr;
    //rd_kafka_topic_t* topic_ = nullptr;
};

