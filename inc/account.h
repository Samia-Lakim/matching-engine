#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <stdint.h>   

#define MAX_NAME_LEN 64

typedef enum {
    ACCOUNT_USER = 0,     
    ACCOUNT_SYSTEM = 1    
} AccountType;

typedef struct Account {
    uint32_t id;               
    char name[MAX_NAME_LEN];   
    AccountType type;          


    int64_t cached_balance;
} Account;

#endif
