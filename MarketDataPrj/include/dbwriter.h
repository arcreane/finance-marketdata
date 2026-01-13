// src/include/dbwriter.h
#pragma once

#include <string>
//#include <libpq-fe.h>
#include "marketdata.h"

class DBWriter {
public:
    explicit DBWriter(const std::string& conninfo);
    ~DBWriter();

    bool insertTick(const MarketDataTick& tick);
    //bool isConnected() const { return conn_ != nullptr; }

private:
    //PGconn* conn_ = nullptr;
};
