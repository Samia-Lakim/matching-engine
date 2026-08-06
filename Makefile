CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinc -g

SRC = lib/order_book.c lib/account.c lib/ledger.c
DEMO_SRC = $(SRC) lib/main.c
TEST_SRC = lib/order_book.c tests/test_order_book.c

BIN_DIR = bin

.PHONY: all demo test clean

all: demo test

demo: $(BIN_DIR)/matching_demo
	./$(BIN_DIR)/matching_demo

test: $(BIN_DIR)/test_order_book
	./$(BIN_DIR)/test_order_book

$(BIN_DIR)/matching_demo: $(DEMO_SRC) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(DEMO_SRC)

$(BIN_DIR)/test_order_book: $(TEST_SRC) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_SRC)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

clean:
	rm -rf $(BIN_DIR)
