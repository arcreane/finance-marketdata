#pragma once

#include <QObject>
#include <QString>
#include <thread>
#include <atomic>

#include <rdkafka.h>

#include "marketdata.h"

// KafkaConsumerQt : consomme le topic "market_data"
// dans un std::thread C++ et émet un signal Qt
// à chaque MarketDataTick reçu.

class KafkaConsumerQt : public QObject
{
    Q_OBJECT

public:
    explicit KafkaConsumerQt(const QString& brokers,
        const QString& topic,
        QObject* parent = nullptr);
    ~KafkaConsumerQt();

    // Lance le thread de consommation
    void start();

    // Arrêt propre
    void stop();

signals:
    void tickReceived(const MarketDataTick& tick);
    void logMessage(const QString& msg);

private:
    void run(); // fonction de thread
    MarketDataTick parseMessage(const std::string& msg);

    QString brokers_;
    QString topic_;

    std::thread worker_;
    std::atomic<bool> running_{ false };

    rd_kafka_t* rk_ = nullptr;
    rd_kafka_conf_t* conf_ = nullptr;
    rd_kafka_topic_partition_list_t* topics_ = nullptr;
};
