#include "ledger.h"
#include <string.h>
#include <stdio.h>
#include <time.h>

void ledger_init(Ledger *l) {
    memset(l, 0, sizeof(Ledger));
    l->account_count = 0;
    l->next_account_id = 1;
    l->entry_count = 0;
    l->next_entry_id = 1;

    /* Every real money movement in or out of the system is modelled as
     * a transfer to/from this account, so deposit()/withdraw() never
     * have to break the double-entry invariant. */
    Account *sys = create_account(l, "SYSTEM");
    l->system_account_id = sys->id;
}

/* Linear scan for a duplicate idempotency key. MAX_ENTRIES is small
 * enough (8192) that this is fine for a teaching/demo project; a real
 * system would index this with a hash map. */
static int idempotency_key_seen(Ledger *l, const char *key) {
    if (key == NULL || key[0] == '\0') {
        return 0; /* no key provided -> no dedup requested */
    }
    for (uint32_t i = 0; i < l->entry_count; i++) {
        if (strncmp(l->entries[i].idempotency_key, key, MAX_IDEMPOTENCY_LEN) == 0) {
            return 1;
        }
    }
    return 0;
}

int post_entry(Ledger *l,
               uint32_t debit_account_id,
               uint32_t credit_account_id,
               int64_t  amount,
               const char *idempotency_key,
               const char *description) {

    if (amount <= 0) {
        return LEDGER_ERR_INVALID_AMOUNT;
    }

    Account *debit_acc  = find_account(l, debit_account_id);
    Account *credit_acc = find_account(l, credit_account_id);
    if (!debit_acc || !credit_acc) {
        return LEDGER_ERR_ACCOUNT_NOT_FOUND;
    }

    if (idempotency_key_seen(l, idempotency_key)) {
        /* Retrying the exact same request (e.g. a client that timed out
         * and resent) is a no-op, not an error the caller needs to
         * handle specially — the operation has already happened. */
        return LEDGER_OK;
    }

    /* System account is allowed to go negative (it represents the
     * outside world); ordinary user accounts cannot overdraw. */
    if (debit_acc->type != ACCOUNT_SYSTEM &&
        debit_acc->cached_balance < amount) {
        return LEDGER_ERR_INSUFFICIENT_FUNDS;
    }

    if (l->entry_count >= MAX_ENTRIES) {
        return LEDGER_ERR_LEDGER_FULL;
    }

    Entry *e = &l->entries[l->entry_count++];
    e->id = l->next_entry_id++;
    e->debit_account_id = debit_account_id;
    e->credit_account_id = credit_account_id;
    e->amount = amount;
    e->timestamp = time(NULL);
    if (idempotency_key) {
        strncpy(e->idempotency_key, idempotency_key, MAX_IDEMPOTENCY_LEN - 1);
        e->idempotency_key[MAX_IDEMPOTENCY_LEN - 1] = '\0';
    } else {
        e->idempotency_key[0] = '\0';
    }
    if (description) {
        strncpy(e->description, description, MAX_DESC_LEN - 1);
        e->description[MAX_DESC_LEN - 1] = '\0';
    } else {
        e->description[0] = '\0';
    }

    /* Update cached balances. These are a performance shortcut only —
     * ledger_verify_integrity() proves they stay consistent with the
     * entry log, which remains the source of truth. */
    debit_acc->cached_balance  -= amount;
    credit_acc->cached_balance += amount;

    return LEDGER_OK;
}

int deposit(Ledger *l, uint32_t account_id, int64_t amount,
            const char *idempotency_key) {
    /* Money enters from outside the system: debit SYSTEM, credit the user. */
    return post_entry(l, l->system_account_id, account_id, amount,
                       idempotency_key, "deposit");
}

int withdraw(Ledger *l, uint32_t account_id, int64_t amount,
             const char *idempotency_key) {
    /* Money leaves the system: debit the user, credit SYSTEM. */
    return post_entry(l, account_id, l->system_account_id, amount,
                       idempotency_key, "withdrawal");
}

int transfer(Ledger *l, uint32_t from_id, uint32_t to_id, int64_t amount,
             const char *idempotency_key) {
    return post_entry(l, from_id, to_id, amount, idempotency_key, "transfer");
}

int ledger_verify_integrity(Ledger *l) {
    int64_t computed[MAX_ACCOUNTS] = {0};

    for (uint32_t i = 0; i < l->entry_count; i++) {
        Entry *e = &l->entries[i];
        for (uint32_t j = 0; j < l->account_count; j++) {
            if (l->accounts[j].id == e->debit_account_id) {
                computed[j] -= e->amount;
            }
            if (l->accounts[j].id == e->credit_account_id) {
                computed[j] += e->amount;
            }
        }
    }

    for (uint32_t j = 0; j < l->account_count; j++) {
        if (computed[j] != l->accounts[j].cached_balance) {
            fprintf(stderr,
                "[INTEGRITY FAILURE] account %u (%s): cached=%lld computed=%lld\n",
                l->accounts[j].id, l->accounts[j].name,
                (long long)l->accounts[j].cached_balance,
                (long long)computed[j]);
            return LEDGER_ERR_INTEGRITY_FAILURE;
        }
    }
    return LEDGER_OK;
}

void print_statement(Ledger *l, uint32_t account_id) {
    Account *a = find_account(l, account_id);
    if (!a) {
        printf("No such account: %u\n", account_id);
        return;
    }

    printf("Statement for account %u (%s)\n", a->id, a->name);
    printf("%-6s %-10s %-10s %12s  %s\n", "ID", "FROM", "TO", "AMOUNT", "DESCRIPTION");
    for (uint32_t i = 0; i < l->entry_count; i++) {
        Entry *e = &l->entries[i];
        if (e->debit_account_id == account_id || e->credit_account_id == account_id) {
            int64_t signed_amount = (e->credit_account_id == account_id) ? e->amount : -e->amount;
            printf("%-6llu %-10u %-10u %12lld  %s\n",
                   (unsigned long long)e->id,
                   e->debit_account_id, e->credit_account_id,
                   (long long)signed_amount, e->description);
        }
    }
    printf("Balance: %lld cents\n", (long long)a->cached_balance);
}
