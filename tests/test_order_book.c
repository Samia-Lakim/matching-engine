#include <stdio.h>
#include "order_book.h"

static int failures = 0;

#define CHECK(cond, msg) do { \
    if (!(cond)) { printf("FAIL: %s\n", msg); failures++; } \
    else { printf("PASS: %s\n", msg); } \
} while (0)

static void test_no_match_when_prices_dont_cross(void) {
    OrderBook book; order_book_init(&book);
    Trade trades[MAX_TRADES_PER_SUBMIT];

    Order sell = { .account_id = 1, .side = SIDE_SELL, .price = 150, .quantity = 10 };
    submit_order(&book, sell, trades);

    Order buy = { .account_id = 2, .side = SIDE_BUY, .price = 140, .quantity = 10 };
    int n = submit_order(&book, buy, trades);

    CHECK(n == 0, "no trade when bid is below ask");
    CHECK(book.bid_count == 1 && book.ask_count == 1, "both orders rest in the book");
}

static void test_simple_full_match(void) {
    OrderBook book; order_book_init(&book);
    Trade trades[MAX_TRADES_PER_SUBMIT];

    Order sell = { .account_id = 1, .side = SIDE_SELL, .price = 150, .quantity = 10 };
    submit_order(&book, sell, trades);

    Order buy = { .account_id = 2, .side = SIDE_BUY, .price = 150, .quantity = 10 };
    int n = submit_order(&book, buy, trades);

    CHECK(n == 1, "exact match produces one trade");
    CHECK(trades[0].quantity == 10, "full quantity traded");
    CHECK(trades[0].price == 150, "trade executes at resting order's price");
    CHECK(book.ask_count == 0 && book.bid_count == 0, "book is empty after full match");
}

static void test_partial_fill_leaves_remainder_resting(void) {
    OrderBook book; order_book_init(&book);
    Trade trades[MAX_TRADES_PER_SUBMIT];

    Order sell = { .account_id = 1, .side = SIDE_SELL, .price = 150, .quantity = 4 };
    submit_order(&book, sell, trades);

    Order buy = { .account_id = 2, .side = SIDE_BUY, .price = 150, .quantity = 10 };
    int n = submit_order(&book, buy, trades);

    CHECK(n == 1, "one trade for available quantity");
    CHECK(trades[0].quantity == 4, "only 4 units traded");
    CHECK(book.ask_count == 0, "seller's order fully consumed");
    CHECK(book.bid_count == 1, "buyer's remaining 6 units rest in the book");
    CHECK(book.bids[0].quantity == 6, "remaining bid quantity is correct");
}

static void test_incoming_order_sweeps_multiple_resting_orders(void) {
    OrderBook book; order_book_init(&book);
    Trade trades[MAX_TRADES_PER_SUBMIT];

    Order sell1 = { .account_id = 1, .side = SIDE_SELL, .price = 150, .quantity = 4 };
    Order sell2 = { .account_id = 2, .side = SIDE_SELL, .price = 151, .quantity = 6 };
    submit_order(&book, sell1, trades);
    submit_order(&book, sell2, trades);

    Order buy = { .account_id = 3, .side = SIDE_BUY, .price = 151, .quantity = 10 };
    int n = submit_order(&book, buy, trades);

    CHECK(n == 2, "sweeps both resting sell orders");
    CHECK(trades[0].price == 150, "first fill at the cheaper resting price");
    CHECK(trades[1].price == 151, "second fill at the next price level");
    CHECK(book.ask_count == 0 && book.bid_count == 0, "book fully cleared");
}

static void test_price_time_priority(void) {
    OrderBook book; order_book_init(&book);
    Trade trades[MAX_TRADES_PER_SUBMIT];

    /* Two sellers at the same price -> whoever rested first should fill first. */
    Order sell1 = { .account_id = 1, .side = SIDE_SELL, .price = 150, .quantity = 5 };
    Order sell2 = { .account_id = 2, .side = SIDE_SELL, .price = 150, .quantity = 5 };
    submit_order(&book, sell1, trades);
    submit_order(&book, sell2, trades);

    Order buy = { .account_id = 3, .side = SIDE_BUY, .price = 150, .quantity = 5 };
    int n = submit_order(&book, buy, trades);

    CHECK(n == 1, "matches only the first resting order");
    CHECK(trades[0].seller_account_id == 1, "earlier resting order (account 1) fills first");
}

int main(void) {
    test_no_match_when_prices_dont_cross();
    test_simple_full_match();
    test_partial_fill_leaves_remainder_resting();
    test_incoming_order_sweeps_multiple_resting_orders();
    test_price_time_priority();

    printf("\n%s\n", failures == 0 ? "ALL TESTS PASSED" : "SOME TESTS FAILED");
    return failures == 0 ? 0 : 1;
}
