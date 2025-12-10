// src/utils/consumer.cpp
#include "consumer.h"
#include <iostream>
#include <sstream>

MarketDataTick Consumer::parseMessage(const std::string& msg)
{
    MarketDataTick t;
    std::stringstream ss(msg);
    std::string token;

    std::getline(ss, t.symbol, ';');
    std::getline(ss, token, ';'); t.last_price = std::stod(token);
    std::getline(ss, token, ';'); t.open = std::stod(token);
    std::getline(ss, token, ';'); t.high = std::stod(token);
    std::getline(ss, token, ';'); t.low = std::stod(token);
    std::getline(ss, token, ';'); t.volume = std::stoll(token);
    std::getline(ss, token, ';'); t.timestamp = std::stoll(token);

    return t;
}

Consumer::Consumer(const std::string& brokers,
    const std::string& topic,
    TickQueue& queue)
    : queue_(queue)
{
    char errstr[512];

    conf_ = rd_kafka_conf_new();

    if (rd_kafka_conf_set(conf_, "bootstrap.servers",
        brokers.c_str(), errstr, sizeof(errstr))
        != RD_KAFKA_CONF_OK)
    {
        std::cerr << "[Consumer] Failed to set bootstrap.servers: "
            << errstr << "\n";
        std::exit(1);
    }

    if (rd_kafka_conf_set(conf_, "group.id", "router-group",
        errstr, sizeof(errstr))
        != RD_KAFKA_CONF_OK)
    {
        std::cerr << "[Consumer] Failed to set group.id: "
            << errstr << "\n";
        std::exit(1);
    }

    rk_ = rd_kafka_new(RD_KAFKA_CONSUMER, conf_, errstr, sizeof(errstr));
    if (!rk_) {
        std::cerr << "[Consumer] Failed to create consumer: "
            << errstr << "\n";
        std::exit(1);
    }

    rd_kafka_poll_set_consumer(rk_);

    topics_ = rd_kafka_topic_partition_list_new(1);
    rd_kafka_topic_partition_list_add(
        topics_, topic.c_str(), RD_KAFKA_PARTITION_UA);

    rd_kafka_resp_err_t err = rd_kafka_subscribe(rk_, topics_);
    if (err != RD_KAFKA_RESP_ERR_NO_ERROR) {
        std::cerr << "[Consumer] Failed to subscribe: "
            << rd_kafka_err2str(err) << "\n";
        std::exit(1);
    }

    std::cout << "[Consumer] Subscribed to " << topic
        << " @ " << brokers << "\n";
}

Consumer::~Consumer()
{
    running_.store(false);

    if (rk_) {
        rd_kafka_consumer_close(rk_);
        rd_kafka_topic_partition_list_destroy(topics_);
        rd_kafka_destroy(rk_);
    }
}

void Consumer::run()
{
    std::cout << "[Consumer] Thread started.\n";

    while (running_.load()) {
        rd_kafka_message_t* msg = rd_kafka_consumer_poll(rk_, 1000);
        if (!msg) continue;

        if (msg->err == RD_KAFKA_RESP_ERR_NO_ERROR) {
            std::string payload(static_cast<char*>(msg->payload), msg->len);
            try {
                MarketDataTick t = parseMessage(payload);
                queue_.push(t);
            }
            catch (const std::exception& e) {
                std::cerr << "[Consumer] Parse error: " << e.what()
                    << " payload=" << payload << "\n";
            }
        }
        else if (msg->err != RD_KAFKA_RESP_ERR__PARTITION_EOF) {
            std::cerr << "[Consumer] Error: "
                << rd_kafka_err2str(msg->err) << "\n";
        }

        rd_kafka_message_destroy(msg);
    }

    std::cout << "[Consumer] Thread stopping.\n";
}

void Consumer::stop()
{
    running_.store(false);
}
