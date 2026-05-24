<<<<<<< HEAD
# Low Latency Order Book

A minimal C++ order book with a FIX parser, ring buffer queue, and object pool. Educational project focused on the hot path: parse → queue → match → rest in book.
=======
# Low-Latency Order Book

A minimal C++ order book with a FIX parser, lock-free ring buffer, memory pool, and price-time priority matching. Educational project focused on the hot path: parse → queue → match → rest in book.
>>>>>>> Add benchmark, trade callback, improved comments"

## Pipeline

```
FIX message → FixParser → RingBuffer → OrderBook (match + limit rest)
<<<<<<< HEAD
=======
                                            ↓
                                       TradeCallback (on every fill)
>>>>>>> Add benchmark, trade callback, improved comments"
```

## Components

| File | Role |
|------|------|
<<<<<<< HEAD
| `Order.h` | `Order`, `PriceLvl`, enums |
| `OrderPool.h` | Pre-allocated order memory (no `new` per order) |
| `OrderBook.h` | Bids/asks, matching, price levels |
| `RingBuffer.h` | SPSC queue between producer and consumer |
| `FixParser.h` | FIX text → `Order` |
| `main.cpp` | Demo scenarios and console output |

## FIX parser — key tags

Fields are separated by SOH (`\x01`). Only **New Order Single** messages are accepted (`35=D`).

| Tag | Field | Description |
|-----|-------|-------------|
| 35 | MsgType | Must be `D` (New Order Single) |
| 11 | ClOrdID | Order ID |
| 44 | Price | Stored as integer ticks (no float) |
=======
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

## FIX parser — key tags

Fields separated by SOH (`\x01`). Only **New Order Single** messages accepted (`35=D`).

| Tag | Field | Notes |
|-----|-------|-------|
| 35 | MsgType | Must be `D` |
| 11 | ClOrdID | Order ID |
| 44 | Price | Integer ticks |
>>>>>>> Add benchmark, trade callback, improved comments"
| 38 | OrderQty | Quantity |
| 54 | Side | `1` = Buy, `2` = Sell |
| 40 | OrdType | `1` = Market, `2` = Limit |

<<<<<<< HEAD
Other tags (e.g. `8=FIX.4.2`) may appear in messages but are ignored.

## Example FIX field order

```
35=D | 11=5 | 44=100 | 38=50 | 54=1 | 40=2
```

In C++ string literals, write SOH so it is not merged with digits: `"44=100\x01" "38=50\x01"` (not `\x0138`).


<img width="688" height="1077" alt="image" src="https://github.com/user-attachments/assets/407e62f5-d13d-42cc-90ad-cb036c5ae269" />


## Build & run

Open `Low latency Order Book.slnx` in Visual Studio, build, and run. The demo prints the book state after each order. Press **DELETE** to exit.

## Matching (short)

- **Buy** hits **asks** at or below the limit price (best ask first).
- **Sell** hits **bids** at or above the limit price (best bid first).
- Unfilled **limit** quantity rests in the book; **market** does not rest after matching.
=======
In C++ string literals, split SOH from digits: `"44=100\x01" "38=50\x01"` — not `\x0138`.

## Matching

- **Buy** hits **asks** at or below the limit price (best ask first).
- **Sell** hits **bids** at or above the limit price (best bid first).
- Unfilled **limit** quantity rests in the book; **market** does not rest.
- Partial fills supported — incoming order walks multiple price levels if needed.

## Build

Open `Low latency Order Book.slnx` in Visual Studio 2022, build Release x64, run. Press **DELETE** to exit.
>>>>>>> Add benchmark, trade callback, improved comments"
