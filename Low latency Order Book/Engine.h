#pragma once
#include "RingBuffer.h"
#include "FixParser.h"
#include "OrderBook.h"


struct Engine
{
	//  Static helper functions are utility; no state needed
	static const char* SideStr(Side s) {
		return s == Side::Buy ? "BUY" : "SELL";
	}

	static const char* TypeStr(OrderType t) {
		return t == OrderType::Limit ? "LIMIT" : "MARKET";
	}

	// Debug output
	static void PrintOrder(const Order& o) {
		std::cout << "   id=" << o.order_id
			<< " | " << SideStr(o.side)
			<< " | " << TypeStr(o.type)
			<< " | price=" << o.price
			<< " | qty=" << o.quantity << std::endl;
	}

	// Visualize current best bid/ask and order count per level
	static void PrintBook(const OrderBook& book) {
		std::cout <<std::endl <<"   --- book ---" << std::endl;
		if (book.bids.empty() && book.asks.empty()) {
			std::cout << "   (empty)" << std::endl;
			return;
		}
		for (const auto& [price, lvl] : book.bids) {
			std::cout << "   BID  " << price << "  qty=" << lvl.total_qty
				<< "  orders=" << lvl.order_count << std::endl;
		}
		for (const auto& [price, lvl] : book.asks) {
			std::cout << "   ASK  " << price << "  qty=" << lvl.total_qty
				<< "  orders=" << lvl.order_count << std::endl;
		}
		std::cout << std::endl;
	}

	// Single-threaded pipeline: parse→enqueue→immediate dequeue→match
	// No cross-thread queue needed for benchmarks; simplifies latency measurement
	static void Submit(OrderBook& book, RingBuffer& queue,
		const char* fix, const char* label)
		{
		Order parsed{};
		// FIX parser decouples wire format from matching engine
		if (!FixParser::Parse(fix, parsed)) {
			return;  // Silently drop on parse error (invalid format)
		}

		// Queued for throughput measurement; immediately retrieved for demonstration.
		if (!queue.Push(parsed)) {
			return;  // Drop on queue full (avoid backpressure in production)
		}

		// Drain queue and process; in production, another thread would consume
		Order from_queue{};
		while (queue.Pop(from_queue)) {
			book.add_order(from_queue.order_id, from_queue.price,
				from_queue.quantity, from_queue.side, from_queue.type);
		}
	}
};