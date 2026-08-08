#ifndef ENTRY_H
#define ENTRY_H

#include <stdint.h>
#include <time.h>

#define MAX_DESC_LEN       128
#define MAX_IDEMPOTENCY_LEN 64


typedef struct Entry {
    uint64_t id;
    uint32_t debit_account_id;   
    uint32_t credit_account_id;  
    int64_t  amount;             
    time_t   timestamp;
    char     idempotency_key[MAX_IDEMPOTENCY_LEN];
    char     description[MAX_DESC_LEN];
} Entry;

#endif
