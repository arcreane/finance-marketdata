#pragma once

#include <QMainWindow>

class QTableView;
class QTextEdit;

#include "DataTableModel.h"
#include "ChartWidget.h"
#include "KafkaConsumerQt.h"
#include "producer.h"   // 🔴 OBLIGATOIRE (Producer utilisé dans le .cpp)

// MainWindow :
// - Table de ticks
// - Chart temps réel
// - Zone de log
// - Lancement Producer Alpha Vantage

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void handleTick(const MarketDataTick& tick);
    void handleLog(const QString& msg);

private:
    // UI
    DataTableModel* model_{ nullptr };
    QTableView* tableView_{ nullptr };
    ChartWidget* chartWidget_{ nullptr };
    QTextEdit* logView_{ nullptr };

    // Kafka Consumer (UI)
    KafkaConsumerQt* kafka_{ nullptr };

    // Alpha Vantage Producer
    Producer* producer_{ nullptr };
};
