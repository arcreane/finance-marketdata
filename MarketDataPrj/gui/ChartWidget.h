#pragma once

#include <QWidget>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

#include "marketdata.h"

QT_BEGIN_NAMESPACE
class QVBoxLayout;
QT_END_NAMESPACE

//QT_CHARTS_USE_NAMESPACE

// Widget de chart simple : affiche l'évolution de last_price
// pour un seul symbole (le dernier reçu)

class ChartWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ChartWidget(QWidget* parent = nullptr);

public slots:
    void onTick(const MarketDataTick& tick);

private:
    QChart* chart_;
    QLineSeries* series_;
    QValueAxis* axisX_;
    QValueAxis* axisY_;
    qint64 pointIndex_ = 0;
};
