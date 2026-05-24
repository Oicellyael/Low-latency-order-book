#pragma once
#include "OrderPool.h"


struct OrderBook
{
	std::map<int64_t, PriceLvl> asks;                          //  Lowest price first (ascending)
	std::map<int64_t, PriceLvl, std::greater<int64_t>> bids;   //  Highest price first (descending)
	OrderPool pool;                                             //  Pre-allocated to avoid malloc on hot path
	TradeCallback on_trade;                                     // Callback decouples matching from event handling

	//  add_order splits into match-then-insert: maximizes fill, then rests unfilled portion
	void add_order(uint64_t order_id, int64_t price, uint64_t quantity, Side side, OrderType type) {
		//  Match first (consume liquidity) before adding to book (become liquidity)
		quantity = match_order(price, side, quantity, order_id);

		//  Market orders don't rest; Limit orders that fully fill also don't rest
		if (quantity == 0 || type == OrderType::Market) return;

		Order* order = pool.acquire();  // O(1) allocation from pre-allocated pool
		if (order == nullptr) {
			std::cerr << "Order pool exhausted!" << std::endl;
			return;
		}
		// Initialize all fields to avoid stale state from pool reuse
		order->order_id = order_id;
		order->price = price;
		order->quantity = quantity;
		order->side = side;
		order->type = type;
		if (order->side == Side::Buy) {
			//  FIFO queue ensures fair execution within same price level
			PriceLvl& lvl = bids[price];
			if (lvl.head == nullptr) {
				lvl.head = order;
				lvl.tail = order;
			}
			else {
				// Append to tail maintains FIFO order
				order->prev = lvl.tail;
				lvl.tail->next = order;
				lvl.tail = order;
			}
			//  Pre-sum to avoid summing all orders when checking if level is empty
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

	// Traverse book from best level (begin()) until price barrier or no quantity left
	uint64_t match_order(int64_t price, Side side, uint64_t quantity, uint64_t order_id) {
		if (side == Side::Buy) {
			// Buy hits asks (sells); lowest ask (begin) is best price for buyer
			while (quantity > 0 && !asks.empty() && price >= asks.begin()->first) {
				PriceLvl& lvl = asks.begin()->second;
				while (quantity > 0 && lvl.head != nullptr) {
					// Fill smaller of incoming qty vs. resting order (partial/full fill)
					uint64_t fill_qty = std::min(quantity, lvl.head->quantity);
					if (on_trade) on_trade(order_id, lvl.head->order_id, price, fill_qty);
					quantity -= fill_qty;
					lvl.head->quantity -= fill_qty;
					lvl.total_qty -= fill_qty;

					if (lvl.head->quantity == 0) {
						Order* filled = lvl.head;
						lvl.head = filled->next;  //  FIFO pop to next resting order
						if (lvl.head != nullptr) lvl.head->prev = nullptr;  //  Unlink old head
						lvl.order_count--;
						pool.release(filled);     //  Return to pool for reuse
					}
				}
				//  Clean up empty price level to avoid stale iteration
				if (lvl.head == nullptr) lvl.tail = nullptr;
				if (lvl.head == nullptr) asks.erase(asks.begin());
			}
		}
		else {
			// Sell hits bids (buys); highest bid (begin) is best price for seller
			while (quantity > 0 && !bids.empty() && price <= bids.begin()->first) {
				PriceLvl& lvl = bids.begin()->second;
				while (quantity > 0 && lvl.head != nullptr) {
					uint64_t fill_qty = std::min(quantity, lvl.head->quantity);
					if (on_trade) on_trade(lvl.head->order_id, order_id, price, fill_qty);
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
		return quantity;  // Leftover goes into book as new resting order (if limit type)
	}
};