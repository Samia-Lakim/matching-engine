#include "ledger.h"
#include <string.h>


Account* create_account(Ledger *l, const char *name) {
    if (l->account_count >= MAX_ACCOUNTS) {
        return NULL;
    }
    Account *a = &l->accounts[l->account_count++];
    a->id = l->next_account_id++;
    strncpy(a->name, name, MAX_NAME_LEN - 1);
    a->name[MAX_NAME_LEN - 1] = '\0';
    a->type = (l->account_count == 1) ? ACCOUNT_SYSTEM : ACCOUNT_USER;
    a->cached_balance = 0;
    return a;
}

Account* find_account(Ledger *l, uint32_t id) {
    for (uint32_t i = 0; i < l->account_count; i++) {
        if (l->accounts[i].id == id) {
            return &l->accounts[i];
        }
    }
    return NULL;
}

int64_t get_balance(Ledger *l, uint32_t account_id) {
    Account *a = find_account(l, account_id);
    return a ? a->cached_balance : 0;
}
