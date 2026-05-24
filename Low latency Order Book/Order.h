#pragma once
#define NOMINMAX
#include <Windows.h>
#include <cstdint>
#include <vector>
#include <iostream>
#include <map>
#include <array>
#include <atomic>
#include <string>
#include <functional>
#include <chrono>

// Why: Using uint8_t for enums minimizes memory footprint in Order struct
enum class Side : uint8_t { Buy, Sell };
enum class OrderType : uint8_t { Limit, Market };

// Why: Function pointer for trade callbacks allows decoupling event handling from matching engine
using TradeCallback = std::function<void(uint64_t buy_id, uint64_t sell_id, int64_t price, uint64_t qty)>;

// Why: Doubly-linked list pointers enable O(1) removal; exact-sized integers ensure deterministic layout
struct Order {
	uint64_t order_id;      // Why: 64-bit ensures unique IDs across billions of orders
	int64_t price;          // Why: Signed supports derivatives; int64_t provides precision
	uint64_t quantity;      // Why: 64-bit supports multi-million unit quantities

	Side side;              // Why: Enum for type-safety (Buy vs Sell) instead of bool
	OrderType type;         // Why: Explicit types prevent matching logic bugs

	Order* prev{ nullptr }; // Why: Doubly-linked enables O(1) removal from order list
	Order* next{ nullptr }; // Why: Forward link enables FIFO traversal at price level
};

// Why: Map-based doubly-linked lists per price level for efficient order management
struct PriceLvl
{
	int64_t	 price;
	uint64_t total_qty{ 0 };    // Why: Pre-summed avoids iterating all orders at level
	uint32_t order_count{ 0 };  // Why: 32-bit sufficient; millions-per-level unrealistic

	Order* head{ nullptr };     // Why: Head pointer for FIFO insertion/traversal
	Order* tail{ nullptr };     // Why: Tail pointer enables O(1) append of new orders
};

