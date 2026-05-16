# Low Latency Order Book

A minimal C++ order book with a FIX parser, ring buffer queue, and object pool. Educational project focused on the hot path: parse → queue → match → rest in book.

## Pipeline

```
FIX message → FixParser → RingBuffer → OrderBook (match + limit rest)
```

## Components

| File | Role |
|------|------|
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
| 38 | OrderQty | Quantity |
| 54 | Side | `1` = Buy, `2` = Sell |
| 40 | OrdType | `1` = Market, `2` = Limit |

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
