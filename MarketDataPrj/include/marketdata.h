// src/include/marketdata.h
#pragma once

#include <string>
#include <cstdint>

struct MarketDataTick {
    std::string  symbol;
    double       last_price = 0.0;
    double       open = 0.0;
    double       high = 0.0;
    double       low = 0.0;
    std::int64_t volume = 0;
    std::int64_t timestamp = 0;   // epoch seconds
};
