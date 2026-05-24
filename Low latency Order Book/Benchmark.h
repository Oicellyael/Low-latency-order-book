#pragma once
#include "OrderBook.h"
#include "Engine.h"
#include <chrono>
#include <iostream>
#include <vector>
#include <array>

// Aggregate result struct avoids per-benchmark printing during measurement
struct BenchmarkResult {
	const char* name;
	long long duration_us;      // Microseconds for sub-millisecond precision
	long long operations;       // Normalize throughput (ops/sec) across tests
	int trades;                 // Track match count for matching test validation
};

// Measure pure add_order performance on Buy side without matching
// Single price level to isolate list-append latency from map traversal
BenchmarkResult BenchmarkAddOrder() {
	OrderBook book;
	book.on_trade = [](uint64_t buy_id, uint64_t sell_id, int64_t price, uint64_t qty) {};

	auto start = std::chrono::high_resolution_clock::now();

	// 100k orders, no matches; measures pool allocation + list insertion
	for (int i = 0; i < 100000; ++i) {
		book.add_order(i + 1, 10000 + (i % 10), 100, Side::Buy, OrderType::Limit);
	}

	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

	return { "add_order", duration.count(), 100000, 0 };
}

//  Measure matching performance: 50k resting + 50k hitting = 50k trades
//  Both at same price level to measure pure matching without map traversal
BenchmarkResult BenchmarkMatchOrder() {
	OrderBook book;
	int trade_count = 0;
	book.on_trade = [&trade_count](uint64_t buy_id, uint64_t sell_id, int64_t price, uint64_t qty) {
		trade_count++;  
	};

	// fill book with resting orders
	for (int i = 0; i < 50000; ++i) {
		book.add_order(i + 1, 10000, 100, Side::Buy, OrderType::Limit);
	}

	auto start = std::chrono::high_resolution_clock::now();

	// incoming sells hit all resting buys.
	for (int i = 0; i < 50000; ++i) {
		book.add_order(100000 + i + 1, 10000, 100, Side::Sell, OrderType::Limit);
	}

	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

	return { "match_order", duration.count(), 50000, trade_count };
}

// Measure end-to-end pipeline: FIX parse + queue + immediate dequeue + match
//  10k orders validate throughput with real message format and callbacks
BenchmarkResult BenchmarkSubmit() {
	Engine engine;
	OrderBook book;
	RingBuffer queue;

	auto start = std::chrono::high_resolution_clock::now();

	// 10k FIX messages; queue used but immediately drained (SPSC simulation)
	for (int i = 0; i < 10000; ++i) {
		Engine::Submit(book, queue,
			"8=FIX.4.2\x01""35=D\x01""11=1\x01""54=1\x01""38=100\x01""44=10000\x01", "");
	}

	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

	return { "Submit", duration.count(), 10000, 0 };
}

//  Batch all results for printing after benchmarks complete
//  Eliminates I/O overhead from measurements; provides clean summary
void RunAllBenchmarks() {
	std::cout << "=== Running Benchmarks ===" << std::endl;

	// fixed size — no heap allocation
	std::array<BenchmarkResult, 3> results = {
		BenchmarkAddOrder(),
		BenchmarkMatchOrder(),
		BenchmarkSubmit()
	};

	// Print after all measurements; avoids I/O jitter during timing
	std::cout << "\n=== Results ===" << std::endl;
	for (const auto& result : results) {
		std::cout << "\n" << result.name << ":" << std::endl;
		std::cout << "   Time: " << result.duration_us << " µs" << std::endl;
		std::cout << "   Ops/sec: " << (result.operations * 1000000.0 / result.duration_us) << std::endl;
		if (result.trades > 0) {
			std::cout << "   Trades: " << result.trades << std::endl;
		}
	}
	std::cout << "\n=== Benchmarks Complete ===" << std::endl;
}