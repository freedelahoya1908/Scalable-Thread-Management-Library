CC      = gcc
CFLAGS  = -Wall -Wextra -pthread -Iinclude -O2
SRC     = src/thread_lifecycle.c src/sync_engine.c src/thread_pool.c
OUT     = thread_library
TEST    = test_runner

all: $(OUT)

$(OUT): $(SRC) main.c
	$(CC) $(CFLAGS) $(SRC) main.c -o $(OUT)

test: $(SRC) tests/test_all.c
	$(CC) $(CFLAGS) $(SRC) tests/test_all.c -o $(TEST)
	./$(TEST)

clean:
	rm -f $(OUT) $(TEST) *.o

.PHONY: all test clean