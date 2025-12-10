#include "MainWindow.h"

#include <QTableView>
#include <QTextEdit>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>

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
