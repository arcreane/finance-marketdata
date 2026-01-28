#pragma once

#include <QMainWindow>
#include <thread>

class QTableView;
class QTextEdit;

#include "DataTableModel.h"
#include "ChartWidget.h"

// ✅ NOUVELLE ARCHITECTURE
#include "consumer.h"
#include "tick_queue.h"

// (Optionnel) Producer si tu l’utilises encore ici
#include "producer.h"

// MainWindow :
// - Table de ticks
// - Chart temps réel
// - Zone de log
// - Consumer Kafka (consumer.cpp)

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private:
    // UI
    DataTableModel* model_{ nullptr };
    QTableView* tableView_{ nullptr };
    ChartWidget* chartWidget_{ nullptr };
    QTextEdit* logView_{ nullptr };

    // ===== Kafka Consumer (NON Qt) =====
    TickQueue tickQueue_;
    Consumer* consumer_{ nullptr };
    std::thread consumerThread_;

    // (Optionnel) Producer si encore utilisé ici
    Producer* producer_{ nullptr };
};
