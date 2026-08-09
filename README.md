# matching-engine

A limit order matching engine written in C, built as a follow-up to my [payment-ledger](https://github.com/Samia-Lakim/payment-ledger) project. Trades that get matched here actually settle through that ledger ... they're not just printed to the console.

## What it does

This is basically a simplified version of what sits at the core of a stock exchange. People submit orders saying "I want to buy X at this price" or "I want to sell X at this price," and the engine's job is to match buyers and sellers together the moment they agree on a price.

It keeps two sorted lists:
- **bids** (buy orders), sorted highest price first
- **asks** (sell orders), sorted lowest price first

When a new order comes in, it checks whether it crosses the best price on the other side of the book. If it does, a trade happens right away. If it doesn't (or there's leftover quantity after a partial match), the remainder just sits in the book, waiting for a future order to match it.

## Why I connected it to the ledger

I built `payment-ledger` first, and initially this was going to be a separate thing. But a matching engine that just says "these two orders matched" and stops isn't actually doing the interesting part , real money still has to move, and it has to move correctly. So instead of writing a second, simpler way to track balances here, every trade this engine produces gets handed off to `transfer()` from the ledger, using the same idempotency keys so nothing can get settled twice by accident.

I kept the two pieces separate on purpose: `submit_order()` just returns a list of `Trade` structs and doesn't touch the ledger at all. That way I can test the matching logic on its own (see `tests/`) without needing a ledger involved, and the ledger doesn't need to know anything about how a trade came to exist.

## Structure

```
inc/
  order.h        one order: side, price, quantity, account, timestamp
  trade.h        one executed trade (output of matching, no ledger logic)
  order_book.h   the book + submit_order()
  account.h, entry.h, ledger.h   from payment-ledger, used for settlement
lib/
  order_book.c   the actual matching logic
  account.c, ledger.c   from payment-ledger
  main.c         demo: builds a small book, matches a trade, settles it
tests/
  test_order_book.c   matching logic tests, no ledger involved
```

## Matching rules (v1)

- One asset, limit orders only , no market orders yet
- Price-time priority: better price wins; if two orders are at the same price, whichever was placed first gets matched first
- One incoming order can match against several resting orders in a row if needed, until it's fully filled or the book runs dry
- A trade executes at the price of the order that was already resting in the book, not the new incoming order — that's how real exchanges do it

## Running it

```
make demo   # runs the full example below and settles it through the ledger
make test   # runs the matching logic tests (17 checks)
```

## Example

1. Bob places a sell order: 10 units at $150. No buyers yet, so it just waits.
2. Alice places a buy order: 10 units, max $148. Doesn't cross Bob's $150, so it also waits.
3. Charlie places a buy order: 10 units, max $151. This crosses Bob's ask — trade executes at $150 (Bob's price), and $1,500 moves from Charlie to Bob through the ledger.

## What's next

- Order cancellation (can't currently pull an order back out of the book)
- Market orders
- More than one asset at a time
- Saving the book/trade history to disk instead of keeping everything in memory
