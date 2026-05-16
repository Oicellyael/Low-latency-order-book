#pragma once
#include <charconv>
#include <string_view>
#include "Order.h"

// Reads FIX text (tag=value fields separated by SOH \x01) into Order
struct FixParser {
	static bool Parse(std::string_view msg, Order& out) {
		out = Order{};
		bool is_new_order = false;

		size_t start = 0;
		while (start < msg.size()) {
			size_t eq = msg.find('=', start);
			if (eq == std::string_view::npos)
				break;

			size_t soh = msg.find('\x01', eq + 1);
			if (soh == std::string_view::npos)
				soh = msg.size();

			auto tag = msg.substr(start, eq - start);
			auto value = msg.substr(eq + 1, soh - eq - 1);
			start = soh + 1;

			if (tag == "35")       // MsgType: D = new order
				is_new_order = (value == "D");
			else if (tag == "11")  // ClOrdID
				ParseU64(value, out.order_id);
			else if (tag == "44")  // Price
				ParseI64(value, out.price);
			else if (tag == "38")  // OrderQty
				ParseU64(value, out.quantity);
			else if (tag == "54") {  // Side: 1=buy 2=sell
				if (value == "1") out.side = Side::Buy;
				else if (value == "2") out.side = Side::Sell;
			}
			else if (tag == "40") {  // OrdType: 1=market 2=limit
				if (value == "1") out.type = OrderType::Market;
				else if (value == "2") out.type = OrderType::Limit;
			}
		}

		return is_new_order && out.order_id != 0 && out.quantity > 0;
	}

private:
	static void ParseU64(std::string_view s, uint64_t& out) {
		std::from_chars(s.data(), s.data() + s.size(), out);
	}

	static void ParseI64(std::string_view s, int64_t& out) {
		std::from_chars(s.data(), s.data() + s.size(), out);
	}
};
