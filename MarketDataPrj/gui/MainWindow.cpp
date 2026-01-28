#include "MainWindow.h"

#include <QTableView>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>
#include <QTimer>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , model_(new DataTableModel(this))
    , tableView_(new QTableView(this))
    , chartWidget_(new ChartWidget(this))
{
    setWindowTitle("Euronext Market Data - Debug GUI");

    // -------------------------------
    // UI setup
    // -------------------------------
    tableView_->setModel(model_);

    auto* central = new QWidget(this);
    auto* vlayout = new QVBoxLayout(central);

    auto* splitter = new QSplitter(Qt::Vertical, central);
    splitter->addWidget(chartWidget_);
    splitter->addWidget(tableView_);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 1);

    vlayout->addWidget(splitter);
    central->setLayout(vlayout);
    setCentralWidget(central);

    // -------------------------------
    // Kafka Consumer (consumer.cpp)
    // -------------------------------
    consumer_ = new Consumer(
        "localhost:19092",
        "market_data",
        tickQueue_
    );

    consumerThread_ = std::thread([this] {
        consumer_->run();
        });

    // -------------------------------
    // Timer Qt : transfert queue → UI
    // -------------------------------
    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this] {
        MarketDataTick tick;
        while (tickQueue_.pop(tick)) {
            model_->addTick(tick);
            chartWidget_->onTick(tick);
        }
        });
    timer->start(100);
}

MainWindow::~MainWindow()
{
    if (consumer_) {
        consumer_->stop();
        if (consumerThread_.joinable())
            consumerThread_.join();
        delete consumer_;
        consumer_ = nullptr;
    }
}
