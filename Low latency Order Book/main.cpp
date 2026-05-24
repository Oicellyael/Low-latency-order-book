#include "OrderBook.h"
#include "FixParser.h"
#include "RingBuffer.h"
#include "Engine.h"
#include "Benchmark.h"

int main() {
	
	RunAllBenchmarks();
	while (!GetAsyncKeyState(VK_DELETE)) {
		Sleep(2000);
	}
	return 0;
	/*
	OrderBook book;
	RingBuffer queue;
	Engine engine;

	book.on_trade = [](uint64_t buy_id, uint64_t sell_id, int64_t price, uint64_t qty) {
		std::cout <<std::endl <<"TRADE: buy=" << buy_id << " sell=" << sell_id
			<< " price=" << price << " qty=" << qty << std::endl;
	};

	engine.Submit(book, queue,"8=FIX.4.2\x01""35=D\x01""11=1\x01""54=1\x01""38=100\x01""44=15075\x01","1) Limit BUY 100 @ 15075");

	engine.Submit(book, queue,"8=FIX.4.2\x01""35=D\x01""11=2\x01""54=2\x01""38=100\x01""44=15075\x01","2) Limit SELL 100 @ 15075");

	engine.PrintBook(book);

	while (!GetAsyncKeyState(VK_DELETE)) {
		Sleep(2000);
	}
	*/
}