#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_MarketDataPrj.h"

class MarketDataPrj : public QMainWindow
{
    Q_OBJECT

public:
    MarketDataPrj(QWidget *parent = nullptr);
    ~MarketDataPrj();

private:
    Ui::MarketDataPrjClass ui;
};

