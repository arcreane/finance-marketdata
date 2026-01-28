#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>

#include <QApplication>

#include <gui/MainWindow.h>
#include "producer.h"
#include "marketdata.h"   // 👈 AJOUT OBLIGATOIRE

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    // 👇 ENREGISTREMENT DU TYPE POUR Qt::QueuedConnection
    qRegisterMetaType<MarketDataTick>("MarketDataTick");

    const char* apiKey = std::getenv("TWELVE_API_KEY");
    if (!apiKey) {
        std::cerr << "TWELVE_API_KEY not set" << std::endl;
        return 1;
    }

    Producer producer(
        "localhost:19092",
        "market_data",
        apiKey
    );

    producer.startTwelveData("BNP.PA", 60);

    MainWindow w;
    w.resize(1200, 800);
    w.show();

    int rc = app.exec();

    producer.stop();
    return rc;
}
