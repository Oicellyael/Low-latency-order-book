#pragma once
#include "OrderPool.h"
struct OrderBook
{
	std::map<int64_t, PriceLvl> asks;                              // sells: lowest price first
	std::map<int64_t, PriceLvl, std::greater<int64_t>> bids;       // buys: highest price first
	OrderPool pool;

	void add_order(uint64_t order_id, int64_t price, uint64_t quantity, Side side, OrderType type) {
		// Try to trade first; quantity = how much is still left
		quantity = match_order(price, side, quantity);

		// Fully filled, or market with nothing left to rest in book
		if (quantity == 0 || type == OrderType::Market) return;

		Order* order = pool.acquire();
		if (order == nullptr) {
			std::cerr << "Order pool exhausted!" << std::endl;
			return;
		}
		order->order_id = order_id;
		order->price = price;
		order->quantity = quantity;
		order->side = side;
		order->type = type;
		if (order->side == Side::Buy) {
			PriceLvl& lvl = bids[price];
			if (lvl.head == nullptr) {
				lvl.head = order;
				lvl.tail = order;
			}
			else {
				order->prev = lvl.tail;
				lvl.tail->next = order;
				lvl.tail = order;
			}
			lvl.total_qty += quantity;
			lvl.order_count++;
		}
		else {
			PriceLvl& lvl = asks[price];
			if (lvl.head == nullptr) {
				lvl.head = order;
				lvl.tail = order;
			}
			else {
				order->prev = lvl.tail;
				lvl.tail->next = order;
				lvl.tail = order;
			}
			lvl.total_qty += quantity;
			lvl.order_count++;
		}
	}
	

	uint64_t match_order(int64_t price, Side side, uint64_t quantity) {
		if (side == Side::Buy) {
			// Buy only hits asks at or below our limit price (best ask = begin())
			while (quantity > 0 && !asks.empty() && price >= asks.begin()->first) {
				PriceLvl& lvl = asks.begin()->second;
				while (quantity > 0 && lvl.head != nullptr) {
					// Trade the smaller of: incoming qty vs front of queue at this price
					uint64_t fill_qty = std::min(quantity, lvl.head->quantity);
					quantity -= fill_qty;
					lvl.head->quantity -= fill_qty;
					lvl.total_qty -= fill_qty;

					if (lvl.head->quantity == 0) {
						Order* filled = lvl.head;
						lvl.head = filled->next;  // next order at same price level
						if (lvl.head != nullptr) lvl.head->prev = nullptr;
						lvl.order_count--;
						pool.release(filled);     // return memory to pool
					}
				}
				if (lvl.head == nullptr) lvl.tail = nullptr;
				if (lvl.head == nullptr) asks.erase(asks.begin());  // price level empty
			}
		}
		else {
			// Sell only hits bids at or above our limit price (best bid = begin())
			while (quantity > 0 && !bids.empty() && price <= bids.begin()->first) {
				PriceLvl& lvl = bids.begin()->second;
				while (quantity > 0 && lvl.head != nullptr) {
					uint64_t fill_qty = std::min(quantity, lvl.head->quantity);
					quantity -= fill_qty;
					lvl.head->quantity -= fill_qty;
					lvl.total_qty -= fill_qty;

					if (lvl.head->quantity == 0) {
						Order* filled = lvl.head;
						lvl.head = filled->next;
						if (lvl.head != nullptr) lvl.head->prev = nullptr;
						lvl.order_count--;
						pool.release(filled);
					}
				}
				if (lvl.head == nullptr) lvl.tail = nullptr;
				if (lvl.head == nullptr) bids.erase(bids.begin());
			}
		}
		return quantity;  // leftover goes into book (if limit order)
	}
};