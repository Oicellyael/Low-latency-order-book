#include "OrderBook.h"
#include "FixParser.h"
#include "RingBuffer.h"

static const char* SideStr(Side s) {
	return s == Side::Buy ? "BUY" : "SELL";
}

static const char* TypeStr(OrderType t) {
	return t == OrderType::Limit ? "LIMIT" : "MARKET";
}

static void PrintOrder(const Order& o) {
	std::cout << "   id=" << o.order_id
		<< " | " << SideStr(o.side)
		<< " | " << TypeStr(o.type)
		<< " | price=" << o.price
		<< " | qty=" << o.quantity << std::endl;
}

static void PrintBook(const OrderBook& book) {
	std::cout << "   --- book ---" << std::endl;
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
}

// One message through the pipeline: FIX string -> parse -> queue -> order book
static void ProcessFix(OrderBook& book, RingBuffer& queue,
	const char* fix, const char* label)
{
	std::cout << "\n>> " << label << std::endl;

	Order parsed{};
	if (!FixParser::Parse(fix, parsed)) {
		std::cout << "   parse: FAILED (not a new order or bad fields)" << std::endl;
		return;
	}
	std::cout << "   parse: OK" << std::endl;
	PrintOrder(parsed);

	if (!queue.Push(parsed)) {
		std::cout << "   queue: FULL" << std::endl;
		return;
	}

	// In real app: another thread would Pop; here we drain right away
	Order from_queue{};
	while (queue.Pop(from_queue)) {
		book.add_order(from_queue.order_id, from_queue.price,
			from_queue.quantity, from_queue.side, from_queue.type);
	}
	PrintBook(book);
}

int main() {
	OrderBook book;
	RingBuffer queue;

	std::cout << "=== Order book demo (FIX -> queue -> match) ===\n";

	// Split \x01 from digits: use "35=D\x01" not "\x0135" (C++ hex escape quirk)

	// Phase 1: resting bids and asks (no cross yet)
	ProcessFix(book, queue,
		"8=FIX.4.2\x01" "35=D\x01" "11=1\x01" "44=99\x01" "38=50\x01" "54=1\x01" "40=2\x01",
		"1) Limit BUY 50 @ 99");

	ProcessFix(book, queue,
		"8=FIX.4.2\x01" "35=D\x01" "11=2\x01" "44=100\x01" "38=40\x01" "54=1\x01" "40=2\x01",
		"2) Limit BUY 40 @ 100");

	ProcessFix(book, queue,
		"8=FIX.4.2\x01" "35=D\x01" "11=3\x01" "44=100\x01" "38=30\x01" "54=1\x01" "40=2\x01",
		"3) Limit BUY 30 @ 100 (same price, 2nd order)");

	ProcessFix(book, queue,
		"8=FIX.4.2\x01" "35=D\x01" "11=4\x01" "44=102\x01" "38=25\x01" "54=2\x01" "40=2\x01",
		"4) Limit SELL 25 @ 102");

	ProcessFix(book, queue,
		"8=FIX.4.2\x01" "35=D\x01" "11=5\x01" "44=101\x01" "38=20\x01" "54=2\x01" "40=2\x01",
		"5) Limit SELL 20 @ 101");

	// Phase 2: sell crosses bids (matching)
	ProcessFix(book, queue,
		"8=FIX.4.2\x01" "35=D\x01" "11=6\x01" "44=100\x01" "38=60\x01" "54=2\x01" "40=2\x01",
		"6) Limit SELL 60 @ 100 -> eats bids @ 100 (70), 10 bid left @ 100");

	ProcessFix(book, queue,
		"8=FIX.4.2\x01" "35=D\x01" "11=7\x01" "44=99\x01" "38=20\x01" "54=2\x01" "40=2\x01",
		"7) Limit SELL 20 @ 99 -> hits bid @ 99");

	// Phase 3: buy crosses asks (matching)
	ProcessFix(book, queue,
		"8=FIX.4.2\x01" "35=D\x01" "11=8\x01" "44=101\x01" "38=15\x01" "54=1\x01" "40=2\x01",
		"8) Limit BUY 15 @ 101 -> matches ask @ 101");

	ProcessFix(book, queue,
		"8=FIX.4.2\x01" "35=D\x01" "11=9\x01" "44=103\x01" "38=10\x01" "54=1\x01" "40=2\x01",
		"9) Limit BUY 10 @ 103 -> walks asks 101 then 102");

	// Phase 4: price too low -> no match, order stays in book
	ProcessFix(book, queue,
		"8=FIX.4.2\x01" "35=D\x01" "11=10\x01" "44=98\x01" "38=5\x01" "54=1\x01" "40=2\x01",
		"10) Limit BUY 5 @ 98 -> no match, rests in book");

	// Not a new order message -> parser should reject
	ProcessFix(book, queue,
		"8=FIX.4.2\x01" "35=8\x01" "11=99\x01",
		"11) Execution report (35=8) -> skip");

	std::cout << "\n=== Done. Press DELETE to exit ===" << std::endl;
	while (!GetAsyncKeyState(VK_DELETE)) {}
	return 0;
}
