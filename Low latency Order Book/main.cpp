#include "OrderBook.h"

int main() {
	OrderBook order_book;

	order_book.add_order(1, 100, 40, Side::Buy, OrderType::Limit);
	order_book.add_order(2, 100, 30, Side::Buy, OrderType::Limit);

	std::cout << "=== After 2 buys ===" << std::endl;
	std::cout << "Bids: " << order_book.bids.size() << " levels" << std::endl;
	for (auto& [price, lvl] : order_book.bids) {
		std::cout << "  Price " << price << " | qty: " << lvl.total_qty << " | orders: " << lvl.order_count << std::endl;
	}
	std::cout << "Asks: " << order_book.asks.size() << " levels" << std::endl;

	std::cout << std::endl;
	order_book.add_order(3, 100, 100, Side::Sell, OrderType::Limit);
    order_book.add_order(4, 100, 30, Side::Buy, OrderType::Limit);
	std::cout << "=== After sell 100 ===" << std::endl;
	std::cout << "Bids: " << order_book.bids.size() << " levels" << std::endl;
	for (auto& [price, lvl] : order_book.bids) {
		std::cout << "  Price " << price << " | qty: " << lvl.total_qty << " | orders: " << lvl.order_count << std::endl;
	}
	std::cout << "Asks: " << order_book.asks.size() << " levels" << std::endl;
	for (auto& [price, lvl] : order_book.asks) {
		std::cout << "  Price " << price << " | qty: " << lvl.total_qty << " | orders: " << lvl.order_count << std::endl;
	}

	while (!GetAsyncKeyState(VK_DELETE)) {}
	return 0;
}