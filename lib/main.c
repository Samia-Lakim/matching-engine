#include <stdio.h>
#include "order_book.h"
#include "ledger.h"

static void settle_trade(Ledger *ledger, Trade *t, int index) {
    char key[MAX_IDEMPOTENCY_LEN];
    snprintf(key, sizeof(key), "trade-%d", index);

    int64_t amount = t->price * t->quantity;
    int status = transfer(ledger, t->buyer_account_id, t->seller_account_id, amount, key);

    printf("Settled trade #%d: %lld @ %lld (buyer %u -> seller %u) : %s\n",
           index, (long long)t->quantity, (long long)t->price,
           t->buyer_account_id, t->seller_account_id,
           status == LEDGER_OK ? "OK" : "FAILED");
}

int main(void) {
    Ledger ledger;
    ledger_init(&ledger);

    Account *alice   = create_account(&ledger, "Alice");
    Account *bob     = create_account(&ledger, "Bob");
    Account *charlie = create_account(&ledger, "Charlie");

    deposit(&ledger, alice->id,   500000, "fund-alice");  
    deposit(&ledger, bob->id,     500000, "fund-bob");
    deposit(&ledger, charlie->id, 500000, "fund-charlie");

    printf("Starting balances: Alice=%lld Bob=%lld Charlie=%lld\n\n",
           (long long)get_balance(&ledger, alice->id),
           (long long)get_balance(&ledger, bob->id),
           (long long)get_balance(&ledger, charlie->id));

    OrderBook book;
    order_book_init(&book);
    Trade trades[MAX_TRADES_PER_SUBMIT];
    int trade_count;
    int trade_index = 0;

    Order bob_sell = { .account_id = bob->id, .side = SIDE_SELL, .price = 15000, .quantity = 10 };
    trade_count = submit_order(&book, bob_sell, trades);
    printf("Bob sells 10 @ 15000 -> %d trade(s)\n", trade_count);
    print_book(&book);

    printf("\n");
    Order alice_buy = { .account_id = alice->id, .side = SIDE_BUY, .price = 14800, .quantity = 10 };
    trade_count = submit_order(&book, alice_buy, trades);
    printf("Alice bids 10 @ 14800 -> %d trade(s)\n", trade_count);
    print_book(&book);

    printf("\n");
    Order charlie_buy = { .account_id = charlie->id, .side = SIDE_BUY, .price = 15100, .quantity = 10 };
    trade_count = submit_order(&book, charlie_buy, trades);
    printf("Charlie bids 10 @ 15100 -> %d trade(s)\n", trade_count);
    for (int i = 0; i < trade_count; i++) {
        settle_trade(&ledger, &trades[i], trade_index++);
    }
    print_book(&book);

    printf("\nFinal balances: Alice=%lld Bob=%lld Charlie=%lld\n",
           (long long)get_balance(&ledger, alice->id),
           (long long)get_balance(&ledger, bob->id),
           (long long)get_balance(&ledger, charlie->id));

    printf("\nIntegrity check: %s\n",
           ledger_verify_integrity(&ledger) == LEDGER_OK ? "OK" : "FAILED");

    printf("\n== Market microstructure ==\n");
    printf("Best bid: %lld, Best ask: %lld, Spread: %lld\n",
        (long long)best_bid(&book), (long long)best_ask(&book), (long long)get_spread(&book));
    printf("Bid depth: %lld, Ask depth: %lld\n",
        (long long)get_depth(&book, SIDE_BUY), (long long)get_depth(&book, SIDE_SELL));

    printf("\n== Market order: Bob sells 10 @ market (no limit price) ==\n");
    Order bob_market_sell = { .account_id = bob->id, .side = SIDE_SELL, .order_type = ORDER_MARKET, .quantity = 10 };
    trade_count = submit_order(&book, bob_market_sell, trades);
    printf("Market sell -> %d trade(s)\n", trade_count);
    for (int i = 0; i < trade_count; i++) {
        settle_trade(&ledger, &trades[i], trade_index++);
    }
    print_book(&book);

    printf("\nFinal balances: Alice=%lld Bob=%lld Charlie=%lld\n",
        (long long)get_balance(&ledger, alice->id),
        (long long)get_balance(&ledger, bob->id),
        (long long)get_balance(&ledger, charlie->id));       

    return 0;
}

