#pragma once

#include <QObject>
#include <QString>
#include <atomic>
#include <thread>

#include <rdkafka.h>

#include "marketdata.h"   // ✅ ICI, PAS DE STRUCT LOCALE

class KafkaConsumerQt : public QObject {
    Q_OBJECT

public:
    explicit KafkaConsumerQt(const QString& brokers,
        const QString& topic,
        QObject* parent = nullptr);

    ~KafkaConsumerQt();

    void start();
    void stop();

signals:
    void tickReceived(const MarketDataTick& tick);
    void logMessage(const QString& msg);

private:
    void run();
    MarketDataTick parseMessage(const std::string& msg);

private:
    QString brokers_;
    QString topic_;

    rd_kafka_t* rk_{ nullptr };
    rd_kafka_conf_t* conf_{ nullptr };
    rd_kafka_topic_partition_list_t* topics_{ nullptr };

    std::atomic<bool> running_{ false };
    std::thread worker_;
};
