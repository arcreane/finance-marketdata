// src/include/tick_queue.h
#pragma once

#include "marketdata.h"
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

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
    std::atomic<bool> stopping_{ false };
};
