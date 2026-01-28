#include "KafkaConsumerQt.h"

#include <sstream>
#include <iostream>
#include <chrono>
#include <thread>

namespace {

    // Helper: wrapper pour conf_set avec logs
    bool set_conf(rd_kafka_conf_t* conf,
        const char* key,
        const std::string& value,
        QString& outErr)
    {
        char errstr[512];
        auto rc = rd_kafka_conf_set(conf, key, value.c_str(), errstr, sizeof(errstr));
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

    // Réglages minimums
    if (!set_conf(conf_, "bootstrap.servers", brokers_.toStdString(), err)) {
        emit logMessage("[KafkaConsumerQt] " + err);
        rd_kafka_conf_destroy(conf_);
        conf_ = nullptr;
        return;
    }

    // Groupe consumer : IMPORTANT (sinon pas de commit/offset propre)
    if (!set_conf(conf_, "group.id", "qt-gui-group", err)) {
        emit logMessage("[KafkaConsumerQt] " + err);
        rd_kafka_conf_destroy(conf_);
        conf_ = nullptr;
        return;
    }

    // Très utile pour tests : lire depuis le début si pas d'offset
    // (sinon "latest" peut donner une impression de vide)
    if (!set_conf(conf_, "auto.offset.reset", "earliest", err)) {
        emit logMessage("[KafkaConsumerQt] " + err);
        rd_kafka_conf_destroy(conf_);
        conf_ = nullptr;
        return;
    }

    // Optionnel : réduire la verbosité Kafka (tu peux commenter si tu veux plus de logs)
    // set_conf(conf_, "log_level", "6", err);

    char errstr[512];
    rk_ = rd_kafka_new(RD_KAFKA_CONSUMER, conf_, errstr, sizeof(errstr));
    if (!rk_) {
        emit logMessage(QString("[KafkaConsumerQt] rd_kafka_new failed: %1").arg(errstr));
        // en cas d'échec, conf_ n'est PAS automatiquement détruite dans tous les cas,
        // mais librdkafka en prend souvent ownership. Pour être safe:
        conf_ = nullptr;
        return;
    }

    // Après rd_kafka_new, librdkafka prend ownership de conf_
    conf_ = nullptr;

    rd_kafka_poll_set_consumer(rk_);

    topics_ = rd_kafka_topic_partition_list_new(1);
    if (!topics_) {
        emit logMessage("[KafkaConsumerQt] rd_kafka_topic_partition_list_new failed");
        rd_kafka_destroy(rk_);
        rk_ = nullptr;
        return;
    }

    rd_kafka_topic_partition_list_add(
        topics_,
        topic_.toStdString().c_str(),
        RD_KAFKA_PARTITION_UA
    );

    auto sub_err = rd_kafka_subscribe(rk_, topics_);
    if (sub_err != RD_KAFKA_RESP_ERR_NO_ERROR) {
        emit logMessage(QString("[KafkaConsumerQt] Subscribe failed: %1")
            .arg(rd_kafka_err2str(sub_err)));
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
        // unsubscribe + close consumer
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
    if (running_.load())
        return;

    if (!rk_) {
        emit logMessage("[KafkaConsumerQt] Cannot start: consumer not initialized (rk_ is null).");
        return;
    }

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
        rd_kafka_message_t* msg = rd_kafka_consumer_poll(rk_, 500);
        if (!msg)
            continue;

        if (msg->err == RD_KAFKA_RESP_ERR_NO_ERROR) {
            // payload
            const char* p = static_cast<const char*>(msg->payload);
            std::string payload(p, p + msg->len);

            try {
                MarketDataTick t = parseMessage(payload);
                emit tickReceived(t);
            }
            catch (const std::exception& e) {
                emit logMessage(QString("[KafkaConsumerQt] parse error: %1 | payload='%2'")
                    .arg(e.what())
                    .arg(QString::fromStdString(payload)));
            }
        }
        else if (msg->err == RD_KAFKA_RESP_ERR__PARTITION_EOF) {
            // fin de partition (pas une erreur bloquante)
            // Tu peux logguer si tu veux:
            // emit logMessage("[KafkaConsumerQt] Reached end of partition");
        }
        else if (msg->err == RD_KAFKA_RESP_ERR__TIMED_OUT) {
            // timeout de poll (pas grave)
        }
        else {
            emit logMessage(QString("[KafkaConsumerQt] Kafka error: %1")
                .arg(rd_kafka_message_errstr(msg)));
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

    // symbol
    if (!std::getline(ss, t.symbol, ';'))
        throw std::runtime_error("missing symbol");

    // last
    if (!std::getline(ss, token, ';'))
        throw std::runtime_error("missing last");
    t.last_price = std::stod(token);

    // open
    if (!std::getline(ss, token, ';'))
        throw std::runtime_error("missing open");
    t.open = std::stod(token);

    // high
    if (!std::getline(ss, token, ';'))
        throw std::runtime_error("missing high");
    t.high = std::stod(token);

    // low
    if (!std::getline(ss, token, ';'))
        throw std::runtime_error("missing low");
    t.low = std::stod(token);

    // volume
    if (!std::getline(ss, token, ';'))
        throw std::runtime_error("missing volume");
    t.volume = std::stoll(token);

    // timestamp
    if (!std::getline(ss, token, ';'))
        throw std::runtime_error("missing timestamp");
    t.timestamp = std::stoll(token);

    return t;
}

