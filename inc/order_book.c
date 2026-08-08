#include "order_book.h"
#include <string.h>
#include <time.h>
#include <stdio.h>

static int64_t min_i64(int64_t a, int64_t b) { return a < b ? a : b; }

void order_book_init(OrderBook *book) {
    memset(book, 0, sizeof(OrderBook));
    book->next_order_id = 1;
}

/* Inserts into bids, kept sorted highest-price-first. On equal price,
 * the new order goes AFTER existing ones at that price (time priority:
 * whoever was resting first gets matched first). */
static void insert_bid(OrderBook *book, Order o) {
    uint32_t i = 0;
    while (i < book->bid_count && book->bids[i].price >= o.price) {
        i++;
    }
    memmove(&book->bids[i + 1], &book->bids[i], (book->bid_count - i) * sizeof(Order));
    book->bids[i] = o;
    book->bid_count++;
}

/* Inserts into asks, kept sorted lowest-price-first, same time-priority rule. */
static void insert_ask(OrderBook *book, Order o) {
    uint32_t i = 0;
    while (i < book->ask_count && book->asks[i].price <= o.price) {
        i++;
    }
    memmove(&book->asks[i + 1], &book->asks[i], (book->ask_count - i) * sizeof(Order));
    book->asks[i] = o;
    book->ask_count++;
}

static void remove_ask_at(OrderBook *book, uint32_t index) {
    memmove(&book->asks[index], &book->asks[index + 1],
            (book->ask_count - index - 1) * sizeof(Order));
    book->ask_count--;
}

static void remove_bid_at(OrderBook *book, uint32_t index) {
    memmove(&book->bids[index], &book->bids[index + 1],
            (book->bid_count - index - 1) * sizeof(Order));
    book->bid_count--;
}

int submit_order(OrderBook *book, Order new_order, Trade *trades_out) {
    new_order.id = book->next_order_id++;
    new_order.timestamp = time(NULL);

    int trade_count = 0;
    int64_t remaining = new_order.quantity;

    if (new_order.side == SIDE_BUY) {
        /* Sweep the ask side while the best ask crosses our limit price. */
        while (remaining > 0 && book->ask_count > 0 &&
               book->asks[0].price <= new_order.price &&
               trade_count < MAX_TRADES_PER_SUBMIT) {

            Order *best_ask = &book->asks[0];
            int64_t matched_qty = min_i64(remaining, best_ask->quantity);

            Trade *t = &trades_out[trade_count++];
            t->buy_order_id       = new_order.id;
            t->sell_order_id      = best_ask->id;
            t->buyer_account_id   = new_order.account_id;
            t->seller_account_id  = best_ask->account_id;
            t->price              = best_ask->price; /* resting order's price wins */
            t->quantity           = matched_qty;
            t->timestamp          = new_order.timestamp;

            remaining -= matched_qty;
            best_ask->quantity -= matched_qty;

            if (best_ask->quantity == 0) {
                remove_ask_at(book, 0);
            }
        }

        if (remaining > 0) {
            new_order.quantity = remaining;
            insert_bid(book, new_order);
        }

    } else { /* SIDE_SELL */
        while (remaining > 0 && book->bid_count > 0 &&
               book->bids[0].price >= new_order.price &&
               trade_count < MAX_TRADES_PER_SUBMIT) {

            Order *best_bid = &book->bids[0];
            int64_t matched_qty = min_i64(remaining, best_bid->quantity);

            Trade *t = &trades_out[trade_count++];
            t->buy_order_id       = best_bid->id;
            t->sell_order_id      = new_order.id;
            t->buyer_account_id   = best_bid->account_id;
            t->seller_account_id  = new_order.account_id;
            t->price              = best_bid->price; /* resting order's price wins */
            t->quantity           = matched_qty;
            t->timestamp          = new_order.timestamp;

            remaining -= matched_qty;
            best_bid->quantity -= matched_qty;

            if (best_bid->quantity == 0) {
                remove_bid_at(book, 0);
            }
        }

        if (remaining > 0) {
            new_order.quantity = remaining;
            insert_ask(book, new_order);
        }
    }

    return trade_count;
}

void print_book(OrderBook *book) {
    printf("--- ORDER BOOK ---\n");
    printf("BIDS (best first)          ASKS (best first)\n");
    uint32_t max_rows = book->bid_count > book->ask_count ? book->bid_count : book->ask_count;
    for (uint32_t i = 0; i < max_rows; i++) {
        char bid_str[32] = "";
        char ask_str[32] = "";
        if (i < book->bid_count) {
            snprintf(bid_str, sizeof(bid_str), "%lld @ %lld",
                      (long long)book->bids[i].quantity, (long long)book->bids[i].price);
        }
        if (i < book->ask_count) {
            snprintf(ask_str, sizeof(ask_str), "%lld @ %lld",
                      (long long)book->asks[i].quantity, (long long)book->asks[i].price);
        }
        printf("%-27s %s\n", bid_str, ask_str);
    }
    if (max_rows == 0) {
        printf("(empty)\n");
    }
}
