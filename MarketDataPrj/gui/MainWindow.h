#pragma once

#include <QMainWindow>

class QTableView;
class QTextEdit;

#include "DataTableModel.h"
#include "ChartWidget.h"
#include "KafkaConsumerQt.h"

// MainWindow :
// - Table de ticks
// - Chart temps réel
// - Zone de log

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
    DataTableModel* model_;
    QTableView* tableView_;
    ChartWidget* chartWidget_;
    QTextEdit* logView_;

    KafkaConsumerQt* kafka_;
};
