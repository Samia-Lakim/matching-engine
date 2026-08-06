# matching-engine

A limit order matching engine in C, built on top of my [payment-ledger](https://github.com/Samia-Lakim/payment-ledger) project — every executed trade settles as a real transfer in a double-entry ledger, not just a printed message.

## What this is

Think of a stock exchange: people submit buy orders and sell orders, and something has to pair them up the moment a buyer and seller agree on a price. That's what this is — a simplified version of that matching logic, for a single asset, all in memory.

The book keeps two sorted lists:
- **bids** (buy orders), highest price first
- **asks** (sell orders), lowest price first

When a new order comes in, it checks whether it crosses the best price on the other side. If it does, a trade happens immediately. If there's quantity left over (or nothing to match against at all), it rests in the book waiting for a future order to match it.

## Why it settles through the ledger

A matching engine on its own can tell you "these two orders matched," but it doesn't actually move any money — it's just deciding *what should happen*. Actually moving money and keeping that auditable is a separate problem, which I'd already solved in my payment-ledger project. So instead of duplicating that logic here, every `Trade` this engine produces gets passed into `transfer()` from the ledger, using the same idempotency keys so a trade can never accidentally get settled twice.

This also keeps the two systems independently testable — I can unit-test the matching logic (`tests/test_order_book.c`) with zero ledger involved, since `submit_order()` just returns a list of `Trade` structs and never touches money itself.

## Project structure

```
inc/
  order.h        A single order: side, price, quantity, account, timestamp
  trade.h        A single executed trade — output of the matcher, not tied to the ledger
  order_book.h   The book itself + submit_order()
  account.h, entry.h, ledger.h   copied from payment-ledger, used for settlement
lib/
  order_book.c   Matching logic
  account.c, ledger.c   copied from payment-ledger
  main.c         Demo: builds a small book, matches a trade, settles it, checks balances
tests/
  test_order_book.c   Matching logic tests (no ledger involved)
```

## Matching rules (v1)

- One asset only, limit orders only (no market orders yet)
- Price-time priority: better prices match first; if two orders share a price, whichever was resting first gets matched first
- An incoming order can sweep through multiple resting orders on the other side until it's fully filled or the book runs out
- Trade executes at the **resting** order's price, not the incoming order's price (standard exchange behaviour)

## Build & run

```
make demo   # builds and runs the demo: matches a trade end-to-end and settles it
make test   # builds and runs the matching logic unit tests
```

## Example: how a trade happens

1. Bob sells 10 units at 150.00 — no buyers yet, so it just rests in the book.
2. Alice bids 10 units at 148.00 — doesn't cross Bob's 150.00, so it also rests.
3. Charlie bids 10 units at 151.00 — this crosses Bob's ask. A trade executes at Bob's price (150.00), Bob's order is removed from the book, and the engine hands off a `Trade` to the ledger, which moves $1,500 from Charlie to Bob.

## What I'd add next

- Order cancellation
- Market orders (match immediately at whatever price is available, no limit)
- Multiple assets/symbols instead of one hardcoded book
- Persisting the order book and trade history to disk
