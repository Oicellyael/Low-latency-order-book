#pragma once
#define NOMINMAX
#include <Windows.h>
#include <cstdint>
#include <vector>
#include <iostream>
#include <map>

enum class Side : uint8_t { Buy, Sell };
enum class OrderType : uint8_t { Limit, Market };

struct Order {
	uint64_t order_id;
	int64_t price;
	uint64_t quantity;

	Side side;
	OrderType type;

	Order* prev{ nullptr };
	Order* next{ nullptr };
};

struct PriceLvl
{
	int64_t	 price;
	uint64_t total_qty{ 0 };
	uint32_t order_count{ 0 };

	Order* head{ nullptr };
	Order* tail{ nullptr };
};

