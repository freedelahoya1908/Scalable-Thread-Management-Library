# Scalable Thread Management Library

A high-performance thread management library implemented in C using POSIX threads.
Supports efficient creation, synchronization, and termination of thousands of concurrent threads.

## Project Description

This project implements a scalable thread management library in C using POSIX threads. The
library is designed for high-performance and concurrent workloads, with APIs for thread lifecycle
control, synchronization primitives, and pooled task execution.

The system focuses on three core goals:
- Efficiently create, manage, and terminate threads with explicit lifecycle tracking.
- Provide safe synchronization using mutexes, semaphores, barriers, and read-write locks.
- Scale to thousands of concurrent tasks through a reusable thread pool and bounded task queue.

The architecture is suitable for compute-heavy applications where throughput, predictable
coordination, and resource-efficient concurrency are critical.

## Modules

- **Module 1 — Thread Lifecycle Manager**: Create, join, detach, cancel and track thread states
- **Module 2 — Synchronization Engine**: Mutex, semaphore, barrier and read-write lock
- **Module 3 — Thread Pool & Scheduler**: Dynamic thread pool with task queue and stats

## Performance

- 18,600+ tasks/second throughput
- Tested with 1000 concurrent tasks
- Zero race conditions — shared counter 1000/1000 ✓

## Build & Run
```bash
# Compile
make

# Run demo
./thread_library

# Run tests
make test

# Run live GUI (browser connected)
python3 server.py
# Then open: http://localhost:8080
```

## Test Results
```
Results: 14 passed | 0 failed
```

## Project Structure
```
thread_library/
├── include/
│   ├── thread_lifecycle.h
│   ├── sync_engine.h
│   └── thread_pool.h
├── src/
│   ├── thread_lifecycle.c
│   ├── sync_engine.c
│   └── thread_pool.c
├── gui/
│   └── dashboard_connected.html
├── tests/
│   └── test_all.c
├── main.c
├── Makefile
├── server.py
└── README.md
```

## Technology Used

| Category | Details |
|---|---|
| Language | C / C++ |
| Threading | POSIX Threads (pthreads) |
| Sync | pthread_mutex, sem, barrier, rwlock |
| GUI | HTML + JavaScript |
| Bridge | Python 3 + WebSocket |
| Build | GCC + Makefile |
| OS | Linux / WSL |