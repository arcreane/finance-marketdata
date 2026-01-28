#include "consumer.h"

#include <nlohmann/json.hpp>
#include <iostream>
#include <sstream>

using json = nlohmann::json;

MarketDataTick Consumer::parseMessage(const std::string& msg)
{
    auto j = json::parse(msg);

    MarketDataTick t;
    t.symbol = j.at("symbol").get<std::string>();
    t.last_price = j.at("last").get<double>();
    t.open = j.at("open").get<double>();
    t.high = j.at("high").get<double>();
    t.low = j.at("low").get<double>();
    t.volume = j.at("volume").get<std::int64_t>();
    t.timestamp = j.at("timestamp").get<std::int64_t>();
    return t;
}

Consumer::Consumer(const std::string& brokers,
    const std::string& topic,
    TickQueue& queue)
    : queue_(queue)
{
    char errstr[512];

    conf_ = rd_kafka_conf_new();
    if (!conf_)
        throw std::runtime_error("rd_kafka_conf_new failed");

    if (rd_kafka_conf_set(conf_, "bootstrap.servers",
        brokers.c_str(), errstr, sizeof(errstr))
        != RD_KAFKA_CONF_OK)
        throw std::runtime_error(errstr);

    if (rd_kafka_conf_set(conf_, "group.id",
        "marketdata-consumer",
        errstr, sizeof(errstr))
        != RD_KAFKA_CONF_OK)
        throw std::runtime_error(errstr);

    rd_kafka_conf_set(conf_, "auto.offset.reset",
        "earliest", nullptr, 0);

    rk_ = rd_kafka_new(RD_KAFKA_CONSUMER, conf_, errstr, sizeof(errstr));
    if (!rk_)
        throw std::runtime_error(errstr);

    conf_ = nullptr;

    rd_kafka_poll_set_consumer(rk_);

    topics_ = rd_kafka_topic_partition_list_new(1);
    rd_kafka_topic_partition_list_add(
        topics_, topic.c_str(), RD_KAFKA_PARTITION_UA);

    rd_kafka_resp_err_t err = rd_kafka_subscribe(rk_, topics_);
    if (err != RD_KAFKA_RESP_ERR_NO_ERROR)
        throw std::runtime_error(rd_kafka_err2str(err));

    running_.store(true);

    std::cout << "[Consumer] Subscribed to " << topic
        << " @ " << brokers << std::endl;
}

Consumer::~Consumer()
{
    stop();

    if (rk_) {
        rd_kafka_consumer_close(rk_);
        rd_kafka_topic_partition_list_destroy(topics_);
        rd_kafka_destroy(rk_);
    }
}

void Consumer::run()
{
    std::cout << "[Consumer] Thread started" << std::endl;

    while (running_.load()) {
        rd_kafka_message_t* msg =
            rd_kafka_consumer_poll(rk_, 1000);

        if (!msg)
            continue;

        if (msg->err == RD_KAFKA_RESP_ERR_NO_ERROR) {
            std::string payload(
                static_cast<const char*>(msg->payload),
                msg->len);

            try {
                MarketDataTick t = parseMessage(payload);
                queue_.push(t);
            }
            catch (const std::exception& e) {
                std::cerr << "[Consumer] Parse error: "
                    << e.what() << std::endl;
            }
        }
        else if (msg->err != RD_KAFKA_RESP_ERR__PARTITION_EOF) {
            std::cerr << "[Consumer] Kafka error: "
                << rd_kafka_message_errstr(msg) << std::endl;
        }

        rd_kafka_message_destroy(msg);
    }

    std::cout << "[Consumer] Thread stopping" << std::endl;
}

void Consumer::stop()
{
    running_.store(false);
}
