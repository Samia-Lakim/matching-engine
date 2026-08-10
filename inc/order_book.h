#ifndef ORDER_BOOK_H
#define ORDER_BOOK_H

#include "order.h"
#include "trade.h"

#define MAX_RESTING_ORDERS 1024
#define MAX_TRADES_PER_SUBMIT 64 

typedef struct OrderBook {
    Order bids[MAX_RESTING_ORDERS]; 
    uint32_t bid_count;

    Order asks[MAX_RESTING_ORDERS]; 
    uint32_t ask_count;

    uint64_t next_order_id;
} OrderBook;

void order_book_init(OrderBook *book);


int submit_order(OrderBook *book, Order new_order, Trade *trades_out);

void print_book(OrderBook *book);

int64_t best_bid(OrderBook *book);
int64_t best_ask(OrderBook *book);
int64_t get_spread(OrderBook *book);
int64_t get_depth(OrderBook *book, Side side);

#endif
