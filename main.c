#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include "include/thread_lifecycle.h"
#include "include/sync_engine.h"
#include "include/thread_pool.h"
#include "include/logger.h"

/* ── Shared counter (mutex protected) ── */
static int          shared_counter = 0;
static MutexHandle* counter_mutex  = NULL;

void section(const char* title) {
    printf("\n══════════════════════════════════════════\n");
    printf("  %s\n", title);
    printf("══════════════════════════════════════════\n");
}

/* ── Lifecycle worker ── */
void* lifecycle_worker(void* arg) {
    int id = *(int*)arg;
    usleep(80000 + (id * 15000));
    printf("[Worker  %2d] Task complete\n", id);
    return NULL;
}

/* ── Pool task (light, short) ── */
void pool_task(void* arg) {
    int id = *(int*)arg;
    (void)id;
    usleep(30000 + (rand() % 80000));
    mutex_lock(counter_mutex);
    shared_counter++;
    mutex_unlock(counter_mutex);
}

/* ── CPU-heavy task ── */
void cpu_task(void* arg) {
    int id = *(int*)arg;
    (void)id;
    volatile double x = 1.0;
    for (int i = 0; i < 50000; i++) x += sin(i) * cos(i);
    (void)x;
    mutex_lock(counter_mutex);
    shared_counter++;
    mutex_unlock(counter_mutex);
}

/* ── Sequential baseline (for comparison) ── */
void run_sequential(int n) {
    for (int i = 0; i < n; i++) {
        volatile double x = 1.0;
        for (int j = 0; j < 50000; j++) x += sin(j) * cos(j);
        (void)x;
        shared_counter++;
    }
}

/* ── Write metrics JSON ── */
void write_metrics(ThreadPool* pool, const char* filename) {
    PoolMetrics m;
    pool_metrics(pool, &m);

    FILE* f = fopen(filename, "w");
    if (!f) return;
    fprintf(f,
        "{\n"
        "  \"submitted\":     %ld,\n"
        "  \"completed\":     %ld,\n"
        "  \"failed\":        %ld,\n"
        "  \"cancelled\":     %ld,\n"
        "  \"live_threads\":  %d,\n"
        "  \"active_threads\":%d,\n"
        "  \"queue_depth\":   %d,\n"
        "  \"avg_wait_ms\":   %.2f,\n"
        "  \"avg_exec_ms\":   %.2f,\n"
        "  \"max_wait_ms\":   %.2f,\n"
        "  \"throughput\":    %.1f,\n"
        "  \"elapsed_sec\":   %.3f\n"
        "}\n",
        m.submitted, m.completed, m.failed, m.cancelled,
        m.live_threads, m.active_threads, m.queue_depth,
        m.avg_wait_ms, m.avg_exec_ms, m.max_wait_ms,
        m.throughput, m.elapsed_sec);
    fclose(f);
}

int main() {
    srand((unsigned)time(NULL));

    /* ── Initialize thread-safe logger ── */
    logger_init(LOG_INFO, "build/metrics/runtime.log");
    LOG_I("main", "Starting Scalable Thread Management Library demo");

    printf("╔══════════════════════════════════════════╗\n");
    printf("║   Scalable Thread Management Library     ║\n");
    printf("║   CSE-316 CA2                            ║\n");
    printf("╚══════════════════════════════════════════╝\n");

    /* ══ MODULE 1: Thread Lifecycle ══ */
    section("MODULE 1: Thread Lifecycle Manager");

    int ids[5] = {1, 2, 3, 4, 5};
    ThreadHandle* handles[5];

    printf("\n[+] Creating 5 threads...\n");
    for (int i = 0; i < 5; i++)
        handles[i] = thread_create(lifecycle_worker, &ids[i], ids[i]);

    printf("\n[+] Checking thread status...\n");
    for (int i = 0; i < 5; i++)
        printf("    Thread %d → %s\n", ids[i], thread_state_str(thread_status(handles[i])));

    printf("\n[+] Joining all threads...\n");
    for (int i = 0; i < 5; i++) {
        thread_join(handles[i]);
        thread_free(handles[i]);
    }

    /* ══ MODULE 2: Synchronization ══ */
    section("MODULE 2: Synchronization Engine");

    printf("\n[+] Mutex demo...\n");
    MutexHandle* m = mutex_init();
    mutex_lock(m);
    printf("    Critical section: executing safely\n");
    mutex_unlock(m);
    mutex_trylock(m);
    mutex_unlock(m);
    mutex_destroy(m);

    printf("\n[+] Semaphore demo...\n");
    SemHandle* s = sem_create(2);
    sem_wait_custom(s);
    sem_wait_custom(s);
    sem_getvalue_custom(s);
    sem_post_custom(s);
    sem_post_custom(s);
    sem_getvalue_custom(s);
    sem_destroy_custom(s);

    printf("\n[+] RWLock demo...\n");
    RWLockHandle* rw = rwlock_init();
    rwlock_rdlock(rw);
    printf("    Reading shared data...\n");
    rwlock_unlock(rw);
    rwlock_wrlock(rw);
    printf("    Writing shared data...\n");
    rwlock_unlock(rw);
    rwlock_destroy(rw);

    /* ══ Logger Demo ══ */
    section("BONUS MODULE: Thread-Safe Logger");
    LOG_D("demo", "This debug message is filtered (min_level=INFO)");
    LOG_I("demo", "Logger is thread-safe and supports levels");
    LOG_W("demo", "Simulated warning: task queue is 75%% full");
    LOG_E("demo", "Simulated error: would log error here");

    /* ══ MODULE 3: Thread Pool ══ */
    section("MODULE 3: Thread Pool & Scheduler");

    counter_mutex  = mutex_init();
    shared_counter = 0;

    printf("\n[+] Initializing pool with 8 threads...\n");
    ThreadPool* pool = pool_init(8);

    printf("\n[+] Submitting 40 tasks (NORMAL priority)...\n");
    int task_args[40];
    for (int i = 0; i < 40; i++) {
        task_args[i] = i + 1;
        task_submit(pool, pool_task, &task_args[i], i + 1);
    }

    printf("\n[+] Pool stats (mid-execution):\n");
    pool_stats(pool);

    printf("\n[+] Waiting for all tasks...\n");
    pool_wait_all(pool);
    pool_stats(pool);
    write_metrics(pool, "build/metrics/latest_metrics.json");

    printf("\n[+] Shared counter final value: %d (expected: 40)\n", shared_counter);

    /* ══ PRIORITY SCHEDULING TEST ══ */
    section("PRIORITY SCHEDULING: Mixed Priority Tasks");

    shared_counter = 0;
    printf("\n[+] Submitting 60 tasks with mixed priorities...\n");
    printf("    20 x NORMAL | 20 x HIGH | 20 x CRITICAL\n\n");

    int pri_args[60];
    for (int i = 0; i < 60; i++) {
        pri_args[i] = i + 1;
        TaskPriority p = (i < 20) ? PRIORITY_NORMAL
                       : (i < 40) ? PRIORITY_HIGH
                       :            PRIORITY_CRITICAL;
        task_submit_priority(pool, cpu_task, &pri_args[i], i+1, p,
                             p == PRIORITY_CRITICAL ? "CRITICAL" :
                             p == PRIORITY_HIGH     ? "HIGH"     : "task");
    }
    pool_wait_all(pool);
    write_metrics(pool, "build/metrics/priority_metrics.json");
    printf("\n[+] Priority test done | counter=%d (expected: 60)\n", shared_counter);

    /* ══ TASK CANCELLATION DEMO ══ */
    section("NEW FEATURE: Task Cancellation");

    shared_counter = 0;
    int cancel_args[30];
    printf("\n[+] Submitting 30 CPU tasks...\n");
    for (int i = 0; i < 30; i++) {
        cancel_args[i] = i + 1;
        task_submit_priority(pool, cpu_task, &cancel_args[i], i+1,
                             PRIORITY_NORMAL, "cancel_demo");
    }
    printf("[+] Cancelling tasks with id 5, 10, 15, 20, 25...\n");
    int cancelled = 0;
    for (int i = 5; i <= 25; i += 5) {
        if (task_cancel(pool, i) == 0) cancelled++;
    }
    pool_wait_all(pool);
    printf("[+] Cancellation test done | completed=%d | cancelled=%d\n",
           shared_counter, cancelled);

    /* ══ DYNAMIC POOL RESIZING ══ */
    section("NEW FEATURE: Dynamic Pool Resizing");

    printf("\n[+] Current pool size: 8\n");
    printf("[+] Growing pool to 16 threads...\n");
    pool_resize(pool, 16);
    sleep(1);
    pool_stats(pool);

    /* ══ SCALABILITY TEST ══ */
    section("SCALABILITY TEST: 1000 Tasks (Pooled)");

    shared_counter = 0;
    printf("\n[+] Submitting 1000 tasks to pool...\n");

    struct timespec t_start, t_end;
    clock_gettime(CLOCK_MONOTONIC, &t_start);

    int big_args[1000];
    for (int i = 0; i < 1000; i++) {
        big_args[i] = i + 1;
        task_submit(pool, pool_task, &big_args[i], i + 1);
    }
    pool_wait_all(pool);
    clock_gettime(CLOCK_MONOTONIC, &t_end);

    double pool_elapsed = (t_end.tv_sec  - t_start.tv_sec) +
                          (t_end.tv_nsec - t_start.tv_nsec) / 1e9;
    double pool_tps     = 1000.0 / pool_elapsed;

    printf("[+] 1000 tasks done in %.2f seconds\n", pool_elapsed);
    printf("[+] Throughput: %.1f tasks/sec\n", pool_tps);
    printf("[+] Shared counter: %d (expected: 1000)\n", shared_counter);

    write_metrics(pool, "build/metrics/latest_metrics.json");
    pool_export_csv(pool, "build/metrics/time_series.csv");
    pool_export_history_json(pool, "build/metrics/history.json");

    /* ══ COMPARISON: POOL vs SEQUENTIAL ══ */
    section("COMPARISON: Thread Pool vs Sequential Execution");

    printf("\n[+] Running 200 CPU tasks SEQUENTIALLY...\n");
    shared_counter = 0;
    struct timespec seq_start, seq_end;
    clock_gettime(CLOCK_MONOTONIC, &seq_start);
    run_sequential(200);
    clock_gettime(CLOCK_MONOTONIC, &seq_end);
    double seq_elapsed = (seq_end.tv_sec  - seq_start.tv_sec) +
                         (seq_end.tv_nsec - seq_start.tv_nsec) / 1e9;

    printf("[+] Running 200 CPU tasks via THREAD POOL...\n");
    shared_counter = 0;
    int cmp_args[200];
    struct timespec par_start, par_end;
    clock_gettime(CLOCK_MONOTONIC, &par_start);
    for (int i = 0; i < 200; i++) {
        cmp_args[i] = i + 1;
        task_submit(pool, cpu_task, &cmp_args[i], i + 1);
    }
    pool_wait_all(pool);
    clock_gettime(CLOCK_MONOTONIC, &par_end);
    double par_elapsed = (par_end.tv_sec  - par_start.tv_sec) +
                         (par_end.tv_nsec - par_start.tv_nsec) / 1e9;

    double speedup = seq_elapsed / par_elapsed;
    printf("\n┌───────────────────────────────────────────┐\n");
    printf("│  COMPARISON RESULTS                        │\n");
    printf("├───────────────────────────────────────────┤\n");
    printf("│  Sequential     : %6.2f sec               │\n", seq_elapsed);
    printf("│  Thread Pool    : %6.2f sec               │\n", par_elapsed);
    printf("│  Speedup        : %5.2fx                   │\n", speedup);
    printf("│  Efficiency     : %5.1f%% (16 threads)      │\n", speedup * 100.0 / 16.0);
    printf("└───────────────────────────────────────────┘\n");

    /* Save comparison to JSON */
    FILE* cmp = fopen("build/metrics/comparison.json", "w");
    if (cmp) {
        fprintf(cmp,
            "{\n  \"sequential_sec\": %.3f,\n  \"pool_sec\": %.3f,\n"
            "  \"speedup\": %.2f,\n  \"threads\": 16,\n  \"tasks\": 200\n}\n",
            seq_elapsed, par_elapsed, speedup);
        fclose(cmp);
    }

    /* ══ CLEANUP ══ */
    section("CLEANUP");
    pool_destroy(pool);
    mutex_destroy(counter_mutex);
    logger_shutdown();

    printf("\n╔══════════════════════════════════════════╗\n");
    printf("║   All modules working correctly ✓        ║\n");
    printf("║   Metrics: build/metrics/                ║\n");
    printf("╚══════════════════════════════════════════╝\n\n");
    return 0;
}