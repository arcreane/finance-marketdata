// src/utils/dbwriter.cpp
#include "dbwriter.h"
#include <iostream>
#include <sstream>

DBWriter::DBWriter(const std::string& conninfo)
{
    conn_ = PQconnectdb(conninfo.c_str());

    if (PQstatus(conn_) != CONNECTION_OK) {
        std::cerr << "[DB] Connection failed: "
            << PQerrorMessage(conn_);
        PQfinish(conn_);
        conn_ = nullptr;
    }
    else {
        std::cout << "[DB] Connected to PostgreSQL.\n";
    }
}

DBWriter::~DBWriter()
{
    if (conn_) {
        PQfinish(conn_);
        conn_ = nullptr;
    }
}

bool DBWriter::insertTick(const MarketDataTick& t)
{
    if (!conn_) return false;

    std::ostringstream ss;
    ss << "INSERT INTO ticks(symbol, ts, last_price, open, high, low, volume)"
        << " VALUES ('"
        << t.symbol << "',"
        << t.timestamp << ","
        << t.last_price << ","
        << t.open << ","
        << t.high << ","
        << t.low << ","
        << t.volume << ")";

    PGresult* res = PQexec(conn_, ss.str().c_str());

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::cerr << "[DB] Insert failed: "
            << PQerrorMessage(conn_) << "\n";
        PQclear(res);
        return false;
    }

    PQclear(res);
    return true;
}

