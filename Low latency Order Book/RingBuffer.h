#pragma once
#include "Order.h"

// SPSC lock-free queue — one producer thread, one consumer thread.
// No mutexes: predictable microsecond latency instead of OS-level blocking.
struct RingBuffer {
    static constexpr size_t Capacity = 1024; // power of 2 for cheap modulo

    std::array<Order, Capacity> buffer{};
    std::atomic<uint64_t> head{ 0 }; // owned by consumer
    std::atomic<uint64_t> tail{ 0 }; // owned by producer

    bool Push(const Order& order) {
        const uint64_t t = tail.load(std::memory_order_relaxed); // producer owns tail, no sync needed
        const uint64_t h = head.load(std::memory_order_acquire); // sync with Pop's release

        if ((t + 1) % Capacity == h) // one slot reserved to distinguish full vs empty
            return false;

        buffer[t % Capacity] = order;
        tail.store(t + 1, std::memory_order_release); // publish after write — consumer sees valid data
        return true;
    }

    bool Pop(Order& order) {
        const uint64_t h = head.load(std::memory_order_relaxed); // consumer owns head
        const uint64_t t = tail.load(std::memory_order_acquire); // sync with Push's release

        if (h == t) // empty
            return false;

        order = buffer[h % Capacity];
        head.store(h + 1, std::memory_order_release); // publish after read
        return true;
    }
};