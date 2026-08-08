#ifndef TRADE_H
#define TRADE_H

#include <stdint.h>
#include <time.h>


typedef struct Trade {
    uint64_t buy_order_id;
    uint64_t sell_order_id;
    uint32_t buyer_account_id;
    uint32_t seller_account_id;
    int64_t  price;      
    int64_t  quantity;   
    time_t   timestamp;
} Trade;

#endif
