#include "KafkaConsumerQt.h"

#include <iostream>
#include <sstream>

KafkaConsumerQt::KafkaConsumerQt(const QString& brokers,
    const QString& topic,
    QObject* parent)
    : QObject(parent)
    , brokers_(brokers)
    , topic_(topic)
{
    char errstr[512];

    conf_ = rd_kafka_conf_new();

    if (rd_kafka_conf_set(conf_,
        "bootstrap.servers",
        brokers_.toStdString().c_str(),
        errstr,
        sizeof(errstr)) != RD_KAFKA_CONF_OK)
    {
        emit logMessage(QString("[KafkaConsumerQt] Failed to set bootstrap.servers: %1")
            .arg(errstr));
        rd_kafka_conf_destroy(conf_);
        conf_ = nullptr;
        return;
    }

    if (rd_kafka_conf_set(conf_,
        "group.id",
        "qt-gui-group",
        errstr,
        sizeof(errstr)) != RD_KAFKA_CONF_OK)
    {
        emit logMessage(QString("[KafkaConsumerQt] Failed to set group.id: %1")
            .arg(errstr));
        rd_kafka_conf_destroy(conf_);
        conf_ = nullptr;
        return;
    }

    rk_ = rd_kafka_new(RD_KAFKA_CONSUMER, conf_, errstr, sizeof(errstr));
    if (!rk_) {
        emit logMessage(QString("[KafkaConsumerQt] Failed to create consumer: %1")
            .arg(errstr));
        conf_ = nullptr; // déjà détruite par rd_kafka_new en cas d'échec
        return;
    }

    rd_kafka_poll_set_consumer(rk_);

    topics_ = rd_kafka_topic_partition_list_new(1);
    rd_kafka_topic_partition_list_add(
        topics_, topic_.toStdString().c_str(), RD_KAFKA_PARTITION_UA);

    rd_kafka_resp_err_t err = rd_kafka_subscribe(rk_, topics_);
    if (err != RD_KAFKA_RESP_ERR_NO_ERROR) {
        emit logMessage(QString("[KafkaConsumerQt] Subscribe failed: %1")
            .arg(rd_kafka_err2str(err)));
    }
    else {
        emit logMessage(QString("[KafkaConsumerQt] Subscribed to %1 @ %2")
            .arg(topic_).arg(brokers_));
    }
}

KafkaConsumerQt::~KafkaConsumerQt()
{
    stop();

    if (rk_) {
        rd_kafka_consumer_close(rk_);
        rd_kafka_topic_partition_list_destroy(topics_);
        rd_kafka_destroy(rk_);
        rk_ = nullptr;
        topics_ = nullptr;
    }
}

void KafkaConsumerQt::start()
{
    if (running_.load() || !rk_) return;

    running_.store(true);
    worker_ = std::thread(&KafkaConsumerQt::run, this);
}

void KafkaConsumerQt::stop()
{
    if (!running_.load()) return;

    running_.store(false);
    if (worker_.joinable())
        worker_.join();
}

void KafkaConsumerQt::run()
{
    emit logMessage("[KafkaConsumerQt] thread started");

    while (running_.load()) {
        rd_kafka_message_t* msg = rd_kafka_consumer_poll(rk_, 500);
        if (!msg)
            continue;

        if (msg->err == RD_KAFKA_RESP_ERR_NO_ERROR) {
            std::string payload(static_cast<char*>(msg->payload), msg->len);
            try {
                MarketDataTick t = parseMessage(payload);
                emit tickReceived(t);  // signal Qt (thread-safe via queued conn)
            }
            catch (const std::exception& e) {
                emit logMessage(QString("[KafkaConsumerQt] parse error: %1")
                    .arg(e.what()));
            }
        }
        else if (msg->err != RD_KAFKA_RESP_ERR__PARTITION_EOF) {
            emit logMessage(QString("[KafkaConsumerQt] error: %1")
                .arg(rd_kafka_err2str(msg->err)));
        }

        rd_kafka_message_destroy(msg);
    }

    emit logMessage("[KafkaConsumerQt] thread stopping");
}

MarketDataTick KafkaConsumerQt::parseMessage(const std::string& msg)
{
    MarketDataTick t;
    std::stringstream ss(msg);
    std::string token;

    std::getline(ss, t.symbol, ';');                // symbol
    std::getline(ss, token, ';'); t.last_price = std::stod(token);
    std::getline(ss, token, ';'); t.open = std::stod(token);
    std::getline(ss, token, ';'); t.high = std::stod(token);
    std::getline(ss, token, ';'); t.low = std::stod(token);
    std::getline(ss, token, ';'); t.volume = std::stoll(token);
    std::getline(ss, token, ';'); t.timestamp = std::stoll(token);

    return t;
}

