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
#include <QApplication>
#include <gui/MainWindow.h>

int main(int argc, char* argv[])
{

    QApplication app(argc, argv);

    MainWindow w;
    w.resize(1200, 800);
    w.show();

    return app.exec();
   
}
