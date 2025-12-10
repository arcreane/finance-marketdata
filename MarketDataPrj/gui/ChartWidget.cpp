#include "ChartWidget.h"

#include <QVBoxLayout>

ChartWidget::ChartWidget(QWidget* parent)
    : QWidget(parent)
    , chart_(new QChart())
    , series_(new QLineSeries())
    , axisX_(new QValueAxis())
    , axisY_(new QValueAxis())
{
    chart_->addSeries(series_);
    chart_->setTitle("Last Price (ticks)");
    chart_->legend()->hide();

    axisX_->setTitleText("Tick #");
    axisY_->setTitleText("Price");

    chart_->addAxis(axisX_, Qt::AlignBottom);
    chart_->addAxis(axisY_, Qt::AlignLeft);

    series_->attachAxis(axisX_);
    series_->attachAxis(axisY_);

    auto* chartView = new QChartView(chart_);
    chartView->setRenderHint(QPainter::Antialiasing);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(chartView);
    setLayout(layout);
}

void ChartWidget::onTick(const MarketDataTick& tick)
{
    series_->append(pointIndex_, tick.last_price);
    pointIndex_++;

    // Ajuste les axes
    axisX_->setRange(std::max<qint64>(0, pointIndex_ - 50), pointIndex_);
    axisY_->setRange(tick.last_price * 0.95, tick.last_price * 1.05);
}
