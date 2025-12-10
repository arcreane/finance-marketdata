// src/main.cpp
#include <iostream>
#include <thread>
#include <chrono>
#include <limits>
#include <ctime>

#include "producer.h"
#include "consumer.h"
#include "tick_queue.h"
#include "dbwriter.h"

int main()
{
    const std::string brokers = "localhost:9092";
    const std::string topic = "market_data";

    const std::string conninfo =
        "dbname=marketdb user=postgres password=admin host=localhost port=5432";

    std::cout << "=== EURONEXT ROUTER C++ (Kafka + PostgreSQL + Threads) ===\n";
    std::cout << "1) Mode Consumer (Kafka -> DB)\n";
    std::cout << "2) Mode Producer de test (envoi de 5 ticks)\n";
    std::cout << "Choix : ";

    int choice = 1;
    std::cin >> choice;

    // ------------------------------------------------------------
    // Producer de test
    // ------------------------------------------------------------
    if (choice == 2)
    {
        Producer producer(brokers, topic);

        for (int i = 0; i < 5; ++i)
        {
            MarketDataTick t;
            t.symbol = "TEST";
            t.last_price = 100.0 + i;
            t.open = 99.0;
            t.high = 101.0;
            t.low = 98.0;
            t.volume = 100000 + i * 1000;
            t.timestamp = std::time(nullptr);

            producer.sendTick(t);
            std::this_thread::sleep_for(std::chrono::milliseconds(400));
        }

        std::cout << "[Main] Ticks de test envoyés.\n";
        return 0;
    }

    // ------------------------------------------------------------
    // Mode Consumer : Kafka -> Queue -> DB
    // ------------------------------------------------------------
    TickQueue queue;
    Consumer consumer(brokers, topic, queue);
    DBWriter db(conninfo);

    if (!db.isConnected())
        std::cerr << "[Main] Warning: PostgreSQL non connecté.\n";

    std::atomic<bool> workerRunning{ true };

    std::thread consumerThread(&Consumer::run, &consumer);

    std::thread workerThread([&] {
        MarketDataTick t;
        while (workerRunning && queue.pop(t)) {
            if (db.isConnected()) {
                if (db.insertTick(t)) {
                    std::cout << "[WORKER] Tick inséré: "
                        << t.symbol << " " << t.last_price << "\n";
                }
                else {
                    std::cout << "[WORKER] INSERT FAILED\n";
                }
            }
            else {
                std::cout << "[WORKER] DB non connectée, tick ignoré\n";
            }
        }
        std::cout << "[WORKER] Thread stopping.\n";
        });

    std::cout << "Consumer en cours. Appuyez sur ENTREE pour arrêter...\n";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();

    std::cout << "[Main] Arrêt demandé...\n";

    workerRunning = false;
    queue.stop();
    consumer.stop();

    if (consumerThread.joinable()) consumerThread.join();
    if (workerThread.joinable())  workerThread.join();

    std::cout << "[Main] Arrêt propre terminé.\n";
    return 0;
}
