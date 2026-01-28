#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>

#include <QApplication>
#include <gui/MainWindow.h>

#include "producer.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    const char* apiKey = std::getenv("TWELVE_API_KEY");
    if (!apiKey) {
        std::cerr << "TWELVE_API_KEY not set" << std::endl;
        return 1;
    }

    Producer producer(
        "localhost:9092",
        "euronext.marketdata",
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
