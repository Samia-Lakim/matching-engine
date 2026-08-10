#ifndef ORDER_H
#define ORDER_H

#include <stdint.h>
#include <time.h>

typedef enum {
    SIDE_BUY = 0,
    SIDE_SELL = 1
} Side;

typedef enum {
    ORDER_LIMIT = 0,
    ORDER_MARKET = 1
} OrderType;

typedef struct Order {
    uint64_t id;
    uint32_t account_id;   
    Side     side;
    int64_t  price;        
    int64_t  quantity;     
    time_t   timestamp;
    OrderType order_type;   
} Order;


#endif
