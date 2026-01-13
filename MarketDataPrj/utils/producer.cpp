// src/utils/producer.cpp
#include "producer.h"
#include <iostream>
#include <sstream>

Producer::Producer(const std::string& brokers, const std::string& topic)
{
   /* char errstr[512];

    conf_ = rd_kafka_conf_new();

    if (rd_kafka_conf_set(conf_, "bootstrap.servers",
        brokers.c_str(), errstr, sizeof(errstr))
        != RD_KAFKA_CONF_OK)
    {
        std::cerr << "[Producer] Failed to set bootstrap.servers: "
            << errstr << "\n";
        std::exit(1);
    }

    rk_ = rd_kafka_new(RD_KAFKA_PRODUCER, conf_, errstr, sizeof(errstr));
    if (!rk_) {
        std::cerr << "[Producer] Failed to create producer: "
            << errstr << "\n";
        std::exit(1);
    }

    topic_ = rd_kafka_topic_new(rk_, topic.c_str(), nullptr);
    if (!topic_) {
        std::cerr << "[Producer] Failed to create topic handle: "
            << rd_kafka_err2str(rd_kafka_last_error()) << "\n";
        std::exit(1);
    }

    std::cout << "[Producer] Ready on " << brokers
        << " topic=" << topic << "\n";*/
}

Producer::~Producer()
{
    //if (rk_) {
    //    rd_kafka_flush(rk_, 3000);
    //    rd_kafka_topic_destroy(topic_);
    //    rd_kafka_destroy(rk_);
    //    // conf_ est libérée par rd_kafka_destroy
    //}
}

void Producer::sendRaw(const std::string& msg)
{
    /*rd_kafka_resp_err_t err = rd_kafka_produce(
        topic_,
        RD_KAFKA_PARTITION_UA,
        RD_KAFKA_MSG_F_COPY,
        const_cast<char*>(msg.data()),
        msg.size(),
        nullptr,
        0,
        nullptr
    );

    if (err != RD_KAFKA_RESP_ERR_NO_ERROR) {
        std::cerr << "[Producer] Produce failed: "
            << rd_kafka_err2str(err) << "\n";
    }

    rd_kafka_poll(rk_, 0);*/
}

void Producer::sendTick(const MarketDataTick& t)
{
    std::ostringstream ss;
    ss << t.symbol << ";"
        << t.last_price << ";"
        << t.open << ";"
        << t.high << ";"
        << t.low << ";"
        << t.volume << ";"
        << t.timestamp;

    sendRaw(ss.str());
}
