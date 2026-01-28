#include "KafkaConsumerQt.h"

#include <nlohmann/json.hpp>

#include <sstream>
#include <iostream>
#include <stdexcept>

using json = nlohmann::json;

namespace {

    // helper pour config Kafka
    bool set_conf(rd_kafka_conf_t* conf,
        const char* key,
        const std::string& value,
        QString& outErr)
    {
        char errstr[512];
        auto rc = rd_kafka_conf_set(conf, key, value.c_str(),
            errstr, sizeof(errstr));
        if (rc != RD_KAFKA_CONF_OK) {
            outErr = QString("rd_kafka_conf_set(%1) failed: %2")
                .arg(key)
                .arg(errstr);
            return false;
        }
        return true;
    }

} // namespace

KafkaConsumerQt::KafkaConsumerQt(const QString& brokers,
    const QString& topic,
    QObject* parent)
    : QObject(parent)
    , brokers_(brokers)
    , topic_(topic)
{
    QString err;

    conf_ = rd_kafka_conf_new();
    if (!conf_) {
        emit logMessage("[KafkaConsumerQt] rd_kafka_conf_new failed");
        return;
    }

    if (!set_conf(conf_, "bootstrap.servers",
        brokers_.toStdString(), err) ||
        !set_conf(conf_, "group.id",
            "qt-gui-group", err) ||
        !set_conf(conf_, "auto.offset.reset",
            "earliest", err)) {

        emit logMessage("[KafkaConsumerQt] " + err);
        rd_kafka_conf_destroy(conf_);
        conf_ = nullptr;
        return;
    }

    char errstr[512];
    rk_ = rd_kafka_new(RD_KAFKA_CONSUMER,
        conf_,
        errstr,
        sizeof(errstr));
    if (!rk_) {
        emit logMessage(QString("[KafkaConsumerQt] rd_kafka_new failed: %1")
            .arg(errstr));
        conf_ = nullptr;
        return;
    }

    // ownership pris par librdkafka
    conf_ = nullptr;

    rd_kafka_poll_set_consumer(rk_);

    topics_ = rd_kafka_topic_partition_list_new(1);
    rd_kafka_topic_partition_list_add(
        topics_,
        topic_.toStdString().c_str(),
        RD_KAFKA_PARTITION_UA);

    auto rc = rd_kafka_subscribe(rk_, topics_);
    if (rc != RD_KAFKA_RESP_ERR_NO_ERROR) {
        emit logMessage(QString("[KafkaConsumerQt] Subscribe failed: %1")
            .arg(rd_kafka_err2str(rc)));
    }
    else {
        emit logMessage(QString("[KafkaConsumerQt] Subscribed to '%1' @ %2")
            .arg(topic_)
            .arg(brokers_));
    }
}

KafkaConsumerQt::~KafkaConsumerQt()
{
    stop();

    if (rk_) {
        rd_kafka_unsubscribe(rk_);
        rd_kafka_consumer_close(rk_);

        if (topics_) {
            rd_kafka_topic_partition_list_destroy(topics_);
            topics_ = nullptr;
        }

        rd_kafka_destroy(rk_);
        rk_ = nullptr;
    }
}

void KafkaConsumerQt::start()
{
    if (running_.load() || !rk_)
        return;

    running_.store(true);
    worker_ = std::thread(&KafkaConsumerQt::run, this);
}

void KafkaConsumerQt::stop()
{
    if (!running_.load())
        return;

    running_.store(false);

    if (worker_.joinable())
        worker_.join();
}

void KafkaConsumerQt::run()
{
    emit logMessage("[KafkaConsumerQt] thread started");

    while (running_.load()) {
        rd_kafka_message_t* msg =
            rd_kafka_consumer_poll(rk_, 500);

        if (!msg)
            continue;

        if (msg->err == RD_KAFKA_RESP_ERR_NO_ERROR) {

            std::string payload(
                static_cast<const char*>(msg->payload),
                msg->len);

            try {
                MarketDataTick tick = parseMessage(payload);
                emit tickReceived(tick);
            }
            catch (const std::exception& e) {
                emit logMessage(QString(
                    "[KafkaConsumerQt] parse error: %1 | payload=%2")
                    .arg(e.what())
                    .arg(QString::fromStdString(payload)));
            }
        }
        else if (msg->err != RD_KAFKA_RESP_ERR__TIMED_OUT &&
            msg->err != RD_KAFKA_RESP_ERR__PARTITION_EOF) {

            emit logMessage(QString("[KafkaConsumerQt] Kafka error: %1")
                .arg(rd_kafka_message_errstr(msg)));
        }

        rd_kafka_message_destroy(msg);
    }

    emit logMessage("[KafkaConsumerQt] thread stopping");
}

MarketDataTick KafkaConsumerQt::parseMessage(const std::string& msg)
{
    auto j = json::parse(msg);

    MarketDataTick t;
    t.symbol = j.at("symbol").get<std::string>();
    t.last_price = j.at("last").get<double>();
    t.open = j.at("open").get<double>();
    t.high = j.at("high").get<double>();
    t.low = j.at("low").get<double>();
    t.volume = j.at("volume").get<long long>();
    t.timestamp = j.at("timestamp").get<long long>();

    return t;
}
