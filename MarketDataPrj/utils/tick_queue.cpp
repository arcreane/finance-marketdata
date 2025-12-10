// src/utils/tick_queue.cpp
#include "tick_queue.h"

void TickQueue::push(const MarketDataTick& t)
{
    {
        std::lock_guard<std::mutex> lock(mtx_);
        queue_.push(t);
    }
    cv_.notify_one();
}

bool TickQueue::pop(MarketDataTick& out)
{
    std::unique_lock<std::mutex> lock(mtx_);
    cv_.wait(lock, [&] { return stopping_ || !queue_.empty(); });

    if (stopping_ && queue_.empty())
        return false;

    out = queue_.front();
    queue_.pop();
    return true;
}

void TickQueue::stop()
{
    stopping_ = true;
    cv_.notify_all();
}
