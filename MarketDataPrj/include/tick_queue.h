#pragma once

#include "marketdata.h"
#include <queue>
#include <mutex>
#include <condition_variable>

class TickQueue {
public:
    TickQueue() = default;

    void push(const MarketDataTick& t);
    bool pop(MarketDataTick& out);   // retourne false si stop() a été appelé
    void stop();

private:
    std::queue<MarketDataTick> queue_;
    std::mutex mtx_;
    std::condition_variable cv_;
    bool stopping_{ false };
};
