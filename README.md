# Low-Latency Order Book

A minimal C++ order book with a FIX parser, lock-free ring buffer, memory pool, and price-time priority matching. Educational project focused on the hot path: parse → queue → match → rest in book.

## Pipeline

```
FIX message → FixParser → RingBuffer → OrderBook (match + limit rest)
                                            ↓
                                       TradeCallback (on every fill)
```

## Components

| File | Role |
|------|------|
| `Order.h` | `Order`, `PriceLvl`, enums, `TradeCallback` |
| `OrderPool.h` | Pre-allocated order memory — no `new` on hot path |
| `OrderBook.h` | Bids/asks map, price-time priority matching, FIFO queues |
| `RingBuffer.h` | SPSC lock-free queue between producer and consumer |
| `FixParser.h` | FIX text → `Order` using `string_view`, no allocations |
| `Engine.h` | Orchestrates FIX → queue → matching pipeline |
| `Benchmark.h` | Throughput measurements for each component |
| `main.cpp` | Demo scenarios and console output |

## Design decisions

**Integer prices** — stored as `int64_t` ticks, never `float`. Floating point comparison is unreliable for financial data (`0.1 + 0.2 != 0.3`).

**Memory pool** — 65536 `Order` objects pre-allocated at startup. Acquire/release is O(1) stack pop/push — no OS interaction on the hot path.

**Intrusive linked list** — `prev/next` pointers embedded in `Order` itself. No separate node allocation; O(1) insert at tail and remove from head within a price level.

**Lock-free ring buffer** — SPSC queue using `std::atomic` with explicit `memory_order_release` / `memory_order_acquire`. No mutexes, no blocking.

**Trade callback** — `std::function` hook fires on every fill. Decouples matching logic from event handling (logging, position tracking, confirmations).

## Benchmarks

Measured on a single thread, Release build:

| Test | Time | Throughput |
|------|------|------------|
| `add_order` (100k orders, no match) | ~650 µs | ~150M ops/sec |
| `match_order` (50k fills) | ~265 µs | ~188M ops/sec |
| `Submit` — full pipeline (10k FIX messages) | ~575 µs | ~17M ops/sec |

`Submit` is ~10x slower than `add_order` due to FIX string parsing. In production systems this is solved with binary protocols (SBE/FAST).


<img width="465" height="573" alt="image" src="https://github.com/user-attachments/assets/c8994bf6-2041-4fa4-8cb8-358a115fd508" />


<img width="449" height="542" alt="image" src="https://github.com/user-attachments/assets/b2c95e8a-b249-40c4-85ac-2207cd0c375b" />


## FIX parser — key tags

Fields separated by SOH (`\x01`). Only **New Order Single** messages accepted (`35=D`).

| Tag | Field | Notes |
|-----|-------|-------|
| 35 | MsgType | Must be `D` |
| 11 | ClOrdID | Order ID |
| 44 | Price | Integer ticks |
| 38 | OrderQty | Quantity |
| 54 | Side | `1` = Buy, `2` = Sell |
| 40 | OrdType | `1` = Market, `2` = Limit |

In C++ string literals, split SOH from digits: `"44=100\x01" "38=50\x01"` — not `\x0138`.

## Matching

- **Buy** hits **asks** at or below the limit price (best ask first).
- **Sell** hits **bids** at or above the limit price (best bid first).
- Unfilled **limit** quantity rests in the book; **market** does not rest.
- Partial fills supported — incoming order walks multiple price levels if needed.

## Build

Open `Low latency Order Book.slnx` in Visual Studio 2022, build Release x64, run. Press **DELETE** to exit.
