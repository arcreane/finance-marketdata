#include "MainWindow.h"

#include <QTableView>
#include <QTextEdit>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>
#include <iostream>
#include <consumer.h>
#include <dbwriter.h>
#include <producer.h>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , model_(new DataTableModel(this))
    , tableView_(new QTableView(this))
    , chartWidget_(new ChartWidget(this))
    , logView_(new QTextEdit(this))
{
    setWindowTitle("Euronext Market Data - Debug GUI");

    tableView_->setModel(model_);

    logView_->setReadOnly(true);

    auto* central = new QWidget(this);
    auto* vlayout = new QVBoxLayout(central);

    auto* splitter = new QSplitter(Qt::Vertical, central);
    splitter->addWidget(chartWidget_);
    splitter->addWidget(tableView_);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 1);

    vlayout->addWidget(splitter);
    vlayout->addWidget(logView_);
    central->setLayout(vlayout);
    setCentralWidget(central);

    // Kafka : même broker / topic que ton backend
    kafka_ = new KafkaConsumerQt("localhost:9092",
        "market_data",
        this);

    QObject::connect(kafka_, &KafkaConsumerQt::tickReceived,
        this, &MainWindow::handleTick);

    QObject::connect(kafka_, &KafkaConsumerQt::logMessage,
        this, &MainWindow::handleLog);

    kafka_->start();


    //////////////////////////////////////////////////////////////////////////////////////////
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
    }

    // ------------------------------------------------------------
    // Mode Consumer : Kafka -> Queue -> DB
    // ------------------------------------------------------------
    TickQueue queue;
    Consumer consumer(brokers, topic, queue);
    DBWriter db(conninfo);

    /* if (!db.isConnected())
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
         });*/

    std::cout << "Consumer en cours. Appuyez sur ENTREE pour arrêter...\n";
    //std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();

    std::cout << "[Main] Arrêt demandé...\n";

    /* workerRunning = false;
     queue.stop();
     consumer.stop();

     if (consumerThread.joinable()) consumerThread.join();
     if (workerThread.joinable())  workerThread.join();*/

    std::cout << "[Main] Arrêt propre terminé.\n";

    //////////////////////////////////////////////////////////////////////////














}

MainWindow::~MainWindow()
{
    if (kafka_) {
        kafka_->stop();
    }
}

void MainWindow::handleTick(const MarketDataTick& tick)
{
    model_->addTick(tick);
    chartWidget_->onTick(tick);
}

void MainWindow::handleLog(const QString& msg)
{
    logView_->append(msg);
}
