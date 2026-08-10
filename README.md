# matching-engine

A limit order matching engine written in C. It plugs into my other project, [payment-ledger](https://github.com/Samia-Lakim/payment-ledger) , when two orders match, the trade actually settles as a real transfer in the ledger, not just a print statement saying "trade happened."

## What this actually does

Think about how a stock exchange works: people send in buy orders and sell orders, and something has to match them up the moment a buyer and a seller agree on a price. That's what this project is a simplified version of that, for one asset, running entirely in memory (no networking, no database, just C structs and arrays).

The order book is just two sorted lists:
- **bids** (buy orders), highest price first
- **asks** (sell orders), lowest price first

When a new order comes in, it checks if it crosses the best price on the other side. If yes, a trade happens right away. If there's nothing to match, or only part of it can be matched, whatever's left just sits in the book waiting for someone else to come along later.

## Two kinds of orders

- **Limit orders** "I'll buy/sell, but only at this price or better." If it can't fully match right away, the rest waits in the book.
- **Market orders** "fill me now, whatever the price is." These skip the price check completely. If there isn't enough to fill it, whatever's left over just gets dropped instead of resting in the book ... there's no price to rest at.

## Checking the state of the book

I also added a few small read-only functions:
- `best_bid()` / `best_ask()` the best price on each side right now
- `get_spread()` the gap between them
- `get_depth()` how much total quantity is sitting on one side

These don't change anything, they just let you look at the book. If a side is empty, they return `-1` since there's no real price to give back.

## Why settlement happens through the ledger, not inside this project

A matching engine on its own only figures out *that* a trade should happen — it doesn't actually move money. I already built that part (with idempotency, double-entry, integrity checks) in my payment-ledger project, so I didn't want to duplicate it here. Instead, every `Trade` this engine produces gets handed off to `transfer()` from the ledger.

This also means I can test the matching logic completely on its own `tests/test_order_book.c` never touches the ledger at all, since `submit_order()` just returns a list of trades and doesn't know or care what happens to them afterward.

## Project structure

```
inc/
  order.h        one order: side, order type, price, quantity, account, timestamp
  trade.h        one executed trade, produced by matching, not tied to the ledger
  order_book.h   the book + submit_order() + the spread/depth helpers
  account.h, entry.h, ledger.h   copied over from payment-ledger, used for settlement
lib/
  order_book.c   all the matching logic
  account.c, ledger.c   copied over from payment-ledger
  main.c         demo: builds a small book, matches trades, settles through the ledger
tests/
  test_order_book.c   tests for the matching logic, no ledger involved
```

## Build & run

```
make demo   # runs the demo end to end
make test   # runs the unit tests
```

## Walking through an example

1. Bob sells 10 units at 150.00 — no buyers yet, so it just sits in the book.
2. Alice bids 10 units at 148.00 — doesn't cross Bob's 150.00, so it also just sits.
3. Charlie bids 10 units at 151.00 — this crosses Bob's price. Trade executes at 150.00 (Bob's price, since he was already resting), Bob's order disappears from the book, and the trade gets settled through the ledger.
4. Then, as a market order example: Bob sells 10 units at market price — it immediately matches whatever's resting on the bid side, no negotiation needed.

## Rules I'm following (v1)

- Only one asset, hardcoded
- Price-time priority : best price wins, and if two orders share a price, whoever got there first gets matched first
- One incoming order can sweep through several resting orders if needed, not just one
- Trades always execute at the resting order's price, never the incoming order's price

