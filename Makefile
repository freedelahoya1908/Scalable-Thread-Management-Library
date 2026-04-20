CC      = gcc
CFLAGS  = -Wall -Wextra -pthread -Iinclude -O2
LDFLAGS = -lm
SRC     = src/thread_lifecycle.c src/sync_engine.c src/thread_pool.c src/logger.c
OUT     = thread_library
TEST    = test_runner

all: prepare $(OUT)

prepare:
	@mkdir -p build/metrics report/graphs

$(OUT): $(SRC) main.c
	$(CC) $(CFLAGS) $(SRC) main.c -o $(OUT) $(LDFLAGS)
	@echo ""
	@echo "✓ Build complete → ./$(OUT)"

test: prepare $(SRC) tests/test_all.c
	$(CC) $(CFLAGS) $(SRC) tests/test_all.c -o $(TEST) $(LDFLAGS)
	./$(TEST)

run: $(OUT)
	./$(OUT)

graphs: $(OUT)
	@echo "→ Running binary to generate metrics..."
	./$(OUT) > /dev/null
	@echo "→ Generating report graphs (matplotlib)..."
	python3 scripts/generate_report_graphs.py
	@echo "✓ Graphs saved to report/graphs/"

server: $(OUT)
	python3 server.py

clean:
	rm -f $(OUT) $(TEST) *.o
	rm -f build/metrics/*.json build/metrics/*.csv build/metrics/*.log

help:
	@echo "Available targets:"
	@echo "  make          → Build the library"
	@echo "  make test     → Run full test suite"
	@echo "  make run      → Run demo program"
	@echo "  make graphs   → Generate report graphs (PNG)"
	@echo "  make server   → Launch live dashboard (localhost:8080)"
	@echo "  make clean    → Remove build artifacts"

.PHONY: all prepare test run graphs server clean help
