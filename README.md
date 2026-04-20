# Scalable Thread Management Library

**C + POSIX Threads | Pthreads API**

A production-grade, scalable thread management library implementing thread lifecycle control, advanced synchronization primitives, a priority-based thread pool, task cancellation, dynamic scaling, thread-safe logging, and a live real-time dashboard with Chart.js visualizations.

---

## ✨ Features

### Core Modules
- ✅ **Thread Lifecycle Manager** — create, join, detach, cancel, status tracking
- ✅ **Synchronization Engine** — Mutex, Semaphore, Barrier, Read-Write Lock
- ✅ **Thread Pool** — fixed or auto-scaling, with priority queue
- ✅ **Priority Scheduling** — NORMAL / HIGH / CRITICAL task levels

### Advanced Features (NEW)
- 🚀 **Task Cancellation** — cancel queued tasks by ID before execution
- 🚀 **Dynamic Pool Resizing** — grow/shrink thread pool at runtime
- 🚀 **Auto-Scaling Mode** — monitor thread adjusts pool based on load
- 🚀 **Thread-Safe Logger** — levels, timestamps, thread-IDs, file output
- 🚀 **Live Time-Series Metrics** — snapshots captured every 250ms
- 🚀 **CSV & JSON Export** — data for reports and external analysis
- 🚀 **Sequential-vs-Pool Benchmark** — quantitative speedup measurement

### Observability
- 📊 **Live Browser Dashboard** — real-time thread grid, terminal, and charts
- 📈 **Live Charts** — throughput, queue depth, completed, live threads (Chart.js)
- 📉 **Report Graphs** — 6 professional matplotlib PNGs for submission
- 🧪 **51 Tests** — comprehensive unit + integration test suite

---

## 🚀 Build & Run

```bash
make              # compile
make run          # run the full demo
make test         # run 51-test suite
make graphs       # generate report graphs (PNG)
make server       # launch live dashboard on http://localhost:8080
make clean        # remove build artifacts
```

### Prerequisites
- GCC with pthread support (`build-essential`)
- Python 3 (for dashboard + report graphs)
- `pip install websockets matplotlib`

---

## 📂 Project Structure

```
Thread_Library_Pro/
├── include/                       # Header files
│   ├── thread_lifecycle.h         # Module 1: thread lifecycle API
│   ├── sync_engine.h              # Module 2: sync primitives
│   ├── thread_pool.h              # Module 3: thread pool + metrics
│   └── logger.h                   # Bonus: thread-safe logger
├── src/                           # Implementations
│   ├── thread_lifecycle.c
│   ├── sync_engine.c
│   ├── thread_pool.c              # Enhanced with cancel/resize/history
│   └── logger.c
├── tests/test_all.c               # 51-test suite
├── gui/
│   └── dashboard_connected.html   # Live dashboard with charts
├── scripts/
│   └── generate_report_graphs.py  # Matplotlib report graphs
├── report/graphs/                 # Generated PNG graphs
│   ├── 01_throughput.png
│   ├── 02_queue_vs_threads.png
│   ├── 03_completed.png
│   ├── 04_wait_time.png
│   ├── 05_comparison.png
│   └── 06_summary_card.png
├── build/metrics/                 # Runtime metrics output
│   ├── latest_metrics.json
│   ├── priority_metrics.json
│   ├── time_series.csv
│   ├── history.json
│   ├── comparison.json
│   └── runtime.log
├── main.c                         # Full-feature demo
├── server.py                      # WebSocket bridge for dashboard
├── Makefile
└── README.md
```

---

## 📘 API Reference

### Thread Lifecycle
```c
ThreadHandle* thread_create(void* (*func)(void*), void* arg, int id);
int           thread_join(ThreadHandle* handle);
int           thread_detach(ThreadHandle* handle);
int           thread_cancel(ThreadHandle* handle);
ThreadState   thread_status(ThreadHandle* handle);
```

### Synchronization
```c
MutexHandle*   mutex_init();          int mutex_lock(MutexHandle* m);
SemHandle*     sem_create(int n);     int sem_wait_custom(SemHandle* s);
BarrierHandle* barrier_init(int n);   int barrier_wait_custom(BarrierHandle* b);
RWLockHandle*  rwlock_init();         int rwlock_rdlock(RWLockHandle* rw);
```

### Thread Pool (Enhanced)
```c
ThreadPool* pool_init(int size);
ThreadPool* pool_init_ex(int size, int min_t, int max_t, int auto_scale);
int         task_submit(ThreadPool* p, void (*f)(void*), void* arg, int id);
int         task_submit_priority(ThreadPool* p, ..., TaskPriority pri, const char* name);
int         task_cancel(ThreadPool* p, int task_id);              // NEW
int         pool_resize(ThreadPool* p, int new_size);             // NEW
int         pool_export_csv(ThreadPool* p, const char* path);     // NEW
int         pool_export_history_json(ThreadPool* p, const char* path); // NEW
```

### Logger
```c
logger_init(LOG_INFO, "runtime.log");
LOG_I("module", "formatted %d message", value);
LOG_W("module", "warning");
LOG_E("module", "error");
```

---

## 📊 Benchmark Results (Observed)

| Metric              | Value                |
|---------------------|----------------------|
| Tasks submitted     | 1,130                |
| Tasks completed     | 1,125                |
| Tasks cancelled     | 5                    |
| Live threads (peak) | 16                   |
| Throughput          | ~193.8 tasks/sec     |
| Avg exec time       | 64.76 ms             |
| Seq vs Pool speedup | **1.77×**            |
| Test suite          | **51 / 51 passing**  |

---

## 🎯 CA2 Rubric Mapping

| Rubric Requirement           | Where Demonstrated                             |
|------------------------------|------------------------------------------------|
| Module-Wise Breakdown        | `include/` and `src/` — 3 clear modules        |
| Functionalities              | See Features section + `main.c` demos          |
| Technology Used              | C, pthreads, POSIX, Python, Chart.js, matplotlib |
| Flow Diagram                 | See diagram in project report                  |
| Revision Tracking (7+ commits) | Maintain in your GitHub repo                 |
| Live Demo                    | `make server` + browser at localhost:8080      |
| Results & Evidence           | `report/graphs/` PNGs, `build/metrics/*`       |

---

## 🧪 Test Coverage (51 tests)

- Module 1 — Thread Lifecycle: 4 tests
- Module 2 — Synchronization: 13 tests (mutex, sem, barrier, rwlock)
- Module 3 — Thread Pool: 7 tests
- Priority Scheduling: 4 tests
- Pool Metrics: 5 tests
- Task Cancellation: 5 tests ⭐ NEW
- Dynamic Resize: 4 tests ⭐ NEW
- Logger: 5 tests ⭐ NEW
- CSV Export: 2 tests ⭐ NEW
- Scalability (500 tasks): 2 tests ⭐ NEW

---

## 🎨 Dashboard Preview

The live dashboard includes:
- **Top metric strip** — 6 key stats updating live
- **Thread visualizer** — animated grid showing thread states
- **Live terminal** — real C binary output with color coding
- **4 live charts** — throughput, completed, queue depth, live threads

Launch with:
```bash
make server
# Open http://localhost:8080
```

---

## 📚 References

1. POSIX Threads Programming — Butenhof, *Programming with POSIX Threads*
2. The Linux Programming Interface — Michael Kerrisk
3. Chart.js documentation — https://www.chartjs.org/
4. Matplotlib documentation — https://matplotlib.org/
5. pthread(7) and pthread_*(3) man pages
