#pragma once
#include "Order.h"

// SPSC queue: one thread Push, one thread Pop (lock-free)
struct RingBuffer {
	static constexpr size_t Capacity = 1024;

	std::array<Order, Capacity> buffer{};
	std::atomic<uint64_t> head{ 0 };
	std::atomic<uint64_t> tail{ 0 };

	bool Push(const Order& order) {
		const uint64_t t = tail.load(std::memory_order_relaxed);
		const uint64_t h = head.load(std::memory_order_acquire);

		if ((t + 1) % Capacity == h)  // full (one slot reserved)
			return false;

		buffer[t % Capacity] = order;
		tail.store(t + 1, std::memory_order_release);  // publish after write
		return true;
	}

	bool Pop(Order& order) {
		const uint64_t h = head.load(std::memory_order_relaxed);
		const uint64_t t = tail.load(std::memory_order_acquire);

		if (h == t)
			return false;

		order = buffer[h % Capacity];
		head.store(h + 1, std::memory_order_release);
		return true;
	}
};
