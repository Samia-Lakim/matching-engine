#ifndef LEDGER_H
#define LEDGER_H

#include "account.h"
#include "entry.h"

#define MAX_ACCOUNTS 1024
#define MAX_ENTRIES  8192

typedef enum {
    LEDGER_OK                    = 0,
    LEDGER_ERR_ACCOUNT_NOT_FOUND = -1,
    LEDGER_ERR_INSUFFICIENT_FUNDS = -2,
    LEDGER_ERR_INVALID_AMOUNT    = -3,
    LEDGER_ERR_DUPLICATE_REQUEST = -4,   
    LEDGER_ERR_LEDGER_FULL       = -5,
    LEDGER_ERR_ACCOUNTS_FULL     = -6,
    LEDGER_ERR_INTEGRITY_FAILURE = -7
} LedgerStatus;

typedef struct Ledger {
    Account  accounts[MAX_ACCOUNTS];
    uint32_t account_count;
    uint32_t next_account_id;

    Entry    entries[MAX_ENTRIES];
    uint32_t entry_count;
    uint64_t next_entry_id;


    uint32_t system_account_id;
} Ledger;

void      ledger_init(Ledger *l);

Account*  create_account(Ledger *l, const char *name);
Account*  find_account(Ledger *l, uint32_t id);
int64_t   get_balance(Ledger *l, uint32_t account_id);


int       post_entry(Ledger *l,
                      uint32_t debit_account_id,
                      uint32_t credit_account_id,
                      int64_t  amount,
                      const char *idempotency_key,
                      const char *description);

int       deposit(Ledger *l, uint32_t account_id, int64_t amount,
                   const char *idempotency_key);
int       withdraw(Ledger *l, uint32_t account_id, int64_t amount,
                    const char *idempotency_key);
int       transfer(Ledger *l, uint32_t from_id, uint32_t to_id, int64_t amount,
                    const char *idempotency_key);

int       ledger_verify_integrity(Ledger *l);

void      print_statement(Ledger *l, uint32_t account_id);

#endif
