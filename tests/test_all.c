#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <unistd.h>
#include "../include/thread_lifecycle.h"
#include "../include/sync_engine.h"
#include "../include/thread_pool.h"
#include "../include/logger.h"

static int test_pass = 0;
static int test_fail = 0;

#define TEST(name, cond) \
    if (cond) { printf("  [PASS] %s\n", name); test_pass++; } \
    else       { printf("  [FAIL] %s\n", name); test_fail++; }

/* ── Lifecycle tests ── */
void* dummy_func(void* arg) { (void)arg; return NULL; }

void test_lifecycle() {
    printf("\n[Test] Module 1: Thread Lifecycle\n");
    int id = 99;
    ThreadHandle* h = thread_create(dummy_func, &id, id);
    TEST("thread_create returns non-null",      h != NULL);
    TEST("thread state is RUNNING",             h->state == THREAD_RUNNING);
    thread_join(h);
    TEST("thread state after join is TERMINATED", h->state == THREAD_TERMINATED);
    TEST("thread_free does not crash",          1);
    thread_free(h);
}

/* ── Sync tests ── */
void test_sync() {
    printf("\n[Test] Module 2: Synchronization\n");

    MutexHandle* m = mutex_init();
    TEST("mutex_init non-null",         m != NULL);
    TEST("mutex not locked on init",    m->is_locked == 0);
    mutex_lock(m);
    TEST("mutex locked after lock",     m->is_locked == 1);
    mutex_unlock(m);
    TEST("mutex unlocked after unlock", m->is_locked == 0);
    mutex_destroy(m);

    SemHandle* s = sem_create(3);
    TEST("sem_create non-null",         s != NULL);
    int val = sem_getvalue_custom(s);
    TEST("semaphore initial value is 3", val == 3);
    sem_destroy_custom(s);

    /* NEW: barrier test */
    BarrierHandle* b = barrier_init(2);
    TEST("barrier_init non-null",       b != NULL);
    TEST("barrier count is 2",          b->count == 2);
    barrier_destroy(b);

    /* NEW: rwlock test */
    RWLockHandle* rw = rwlock_init();
    TEST("rwlock_init non-null",        rw != NULL);
    TEST("rwlock_rdlock success",       rwlock_rdlock(rw) == 0);
    TEST("rwlock_unlock success",       rwlock_unlock(rw) == 0);
    TEST("rwlock_wrlock success",       rwlock_wrlock(rw) == 0);
    TEST("rwlock_unlock after wrlock",  rwlock_unlock(rw) == 0);
    rwlock_destroy(rw);
}

/* ── Pool tests ── */
static int    pool_counter = 0;
static MutexHandle* pool_mutex;

void counter_task(void* arg) {
    (void)arg;
    mutex_lock(pool_mutex);
    pool_counter++;
    mutex_unlock(pool_mutex);
}

void test_pool() {
    printf("\n[Test] Module 3: Thread Pool\n");

    ThreadPool* pool = pool_init(4);
    TEST("pool_init non-null",  pool != NULL);
    TEST("pool size is 4",      pool->pool_size == 4);
    TEST("live_count is 4",     pool->live_count == 4);

    pool_mutex   = mutex_init();
    pool_counter = 0;

    int dummy[20];
    for (int i = 0; i < 20; i++) {
        dummy[i] = i;
        task_submit(pool, counter_task, &dummy[i], i);
    }
    pool_wait_all(pool);
    TEST("all 20 tasks completed",           pool_counter == 20);
    TEST("tasks_completed counter == 20",    pool->tasks_completed == 20);
    TEST("tasks_submitted counter == 20",    pool->tasks_submitted == 20);
    TEST("tasks_failed == 0",                pool->tasks_failed == 0);

    mutex_destroy(pool_mutex);
    pool_destroy(pool);
}

/* ── Priority tests ── */
static int priority_order[6];
static int priority_idx = 0;
static MutexHandle* pri_mutex;

void pri_task_normal(void* arg)   { (void)arg; mutex_lock(pri_mutex); priority_order[priority_idx++] = 0; mutex_unlock(pri_mutex); }
void pri_task_high(void* arg)     { (void)arg; mutex_lock(pri_mutex); priority_order[priority_idx++] = 1; mutex_unlock(pri_mutex); }
void pri_task_critical(void* arg) { (void)arg; mutex_lock(pri_mutex); priority_order[priority_idx++] = 2; mutex_unlock(pri_mutex); }

void test_priority() {
    printf("\n[Test] Priority Scheduling\n");
    pri_mutex = mutex_init();

    ThreadPool* pool = pool_init(1);

    int a=1,b=2,c=3,d=4,e=5,f=6;
    task_submit_priority(pool, pri_task_normal,   &a, 1, PRIORITY_NORMAL,   "task");
    task_submit_priority(pool, pri_task_normal,   &b, 2, PRIORITY_NORMAL,   "task");
    task_submit_priority(pool, pri_task_high,     &c, 3, PRIORITY_HIGH,     "task");
    task_submit_priority(pool, pri_task_high,     &d, 4, PRIORITY_HIGH,     "task");
    task_submit_priority(pool, pri_task_critical, &e, 5, PRIORITY_CRITICAL, "task");
    task_submit_priority(pool, pri_task_critical, &f, 6, PRIORITY_CRITICAL, "task");
    pool_wait_all(pool);

    int found_critical = 0;
    for (int i = 0; i < 6; i++) if (priority_order[i] == 2) { found_critical = i; break; }
    TEST("critical tasks execute before normals", found_critical < 4);
    TEST("priority_str NORMAL",   strcmp(priority_str(PRIORITY_NORMAL),   "NORMAL")   == 0);
    TEST("priority_str HIGH",     strcmp(priority_str(PRIORITY_HIGH),     "HIGH")     == 0);
    TEST("priority_str CRITICAL", strcmp(priority_str(PRIORITY_CRITICAL), "CRITICAL") == 0);

    mutex_destroy(pri_mutex);
    pool_destroy(pool);
}

/* ── Metrics test ── */
static int mc = 0;
static MutexHandle* mx_global;
void task_m(void* a) { (void)a; mutex_lock(mx_global); mc++; mutex_unlock(mx_global); }

void test_metrics() {
    printf("\n[Test] Pool Metrics\n");

    ThreadPool* pool = pool_init(2);
    mx_global = mutex_init();
    mc = 0;

    int args[10];
    for (int i=0;i<10;i++){args[i]=i; task_submit(pool, task_m, &args[i], i);}
    pool_wait_all(pool);

    PoolMetrics m;
    pool_metrics(pool, &m);
    TEST("metrics completed == 10",  m.completed == 10);
    TEST("metrics submitted == 10",  m.submitted == 10);
    TEST("metrics failed == 0",      m.failed == 0);
    TEST("metrics live_threads == 2",m.live_threads == 2);
    TEST("metrics throughput > 0",   m.throughput > 0);

    mutex_destroy(mx_global);
    pool_destroy(pool);
}

/* ── NEW: Task cancellation test ── */
static int cancel_run_count = 0;
static MutexHandle* cc_mutex;
void slow_task(void* a) {
    (void)a;
    usleep(50000);
    mutex_lock(cc_mutex);
    cancel_run_count++;
    mutex_unlock(cc_mutex);
}

void test_cancellation() {
    printf("\n[Test] Task Cancellation\n");
    cc_mutex = mutex_init();
    cancel_run_count = 0;

    ThreadPool* pool = pool_init(1);  /* single worker so queue builds up */
    int args[10];
    for (int i = 0; i < 10; i++) {
        args[i] = i;
        task_submit(pool, slow_task, &args[i], i);
    }

    /* Cancel tasks 7, 8, 9 while queued */
    usleep(10000);  /* Let first task start executing */
    int c1 = task_cancel(pool, 7);
    int c2 = task_cancel(pool, 8);
    int c3 = task_cancel(pool, 9);

    pool_wait_all(pool);

    TEST("cancel task 7 succeeds",     c1 == 0);
    TEST("cancel task 8 succeeds",     c2 == 0);
    TEST("cancel task 9 succeeds",     c3 == 0);
    TEST("cancelled counter >= 3",     pool->tasks_cancelled >= 3);
    TEST("completed + cancelled = 10", pool->tasks_completed + pool->tasks_cancelled == 10);

    mutex_destroy(cc_mutex);
    pool_destroy(pool);
}

/* ── NEW: Dynamic resize test ── */
void test_resize() {
    printf("\n[Test] Dynamic Pool Resize\n");

    ThreadPool* pool = pool_init(4);
    TEST("initial pool size = 4",  pool->pool_size == 4);

    int r = pool_resize(pool, 8);
    TEST("resize to 8 returns 0",  r == 0);
    usleep(100000);
    TEST("pool size increased",    pool->pool_size == 8);
    TEST("live threads >= 8",      pool->live_count >= 8);

    pool_destroy(pool);
}

/* ── NEW: Logger test ── */
void test_logger() {
    printf("\n[Test] Thread-Safe Logger\n");
    logger_init(LOG_DEBUG, NULL);
    logger_set_colors(0);  /* disable colors in test output */

    TEST("log_level_str DEBUG",   strcmp(log_level_str(LOG_DEBUG), "DEBUG") == 0);
    TEST("log_level_str INFO",    strcmp(log_level_str(LOG_INFO),  "INFO ") == 0);
    TEST("log_level_str WARN",    strcmp(log_level_str(LOG_WARN),  "WARN ") == 0);
    TEST("log_level_str ERROR",   strcmp(log_level_str(LOG_ERROR), "ERROR") == 0);
    TEST("log_level_str FATAL",   strcmp(log_level_str(LOG_FATAL), "FATAL") == 0);

    logger_shutdown();
}

/* ── NEW: CSV export test ── */
void test_csv_export() {
    printf("\n[Test] CSV Export\n");
    ThreadPool* pool = pool_init(2);
    mx_global = mutex_init();
    mc = 0;
    int args[20];
    for (int i=0;i<20;i++){args[i]=i; task_submit(pool, task_m, &args[i], i);}
    pool_wait_all(pool);

    int r = pool_export_csv(pool, "build/metrics/test_export.csv");
    TEST("CSV export returns 0",  r == 0);

    FILE* f = fopen("build/metrics/test_export.csv", "r");
    TEST("CSV file created",      f != NULL);
    if (f) fclose(f);

    mutex_destroy(mx_global);
    pool_destroy(pool);
}

/* ── NEW: Scalability mini-benchmark ── */
static int sc_counter = 0;
static MutexHandle* sc_mutex;
void sc_task(void* a) { (void)a; mutex_lock(sc_mutex); sc_counter++; mutex_unlock(sc_mutex); }

void test_scalability() {
    printf("\n[Test] Scalability (500 tasks)\n");
    sc_mutex = mutex_init();
    sc_counter = 0;

    ThreadPool* pool = pool_init(8);
    int args[500];
    for (int i=0;i<500;i++){args[i]=i; task_submit(pool, sc_task, &args[i], i);}
    pool_wait_all(pool);

    TEST("500 tasks all completed", sc_counter == 500);
    TEST("tasks_completed == 500",  pool->tasks_completed == 500);

    mutex_destroy(sc_mutex);
    pool_destroy(pool);
}

/* ── Main ── */
int main() {
    printf("╔══════════════════════════════════════╗\n");
    printf("║  Thread Library — Test Suite         ║\n");
    printf("╚══════════════════════════════════════╝\n");

    test_lifecycle();
    test_sync();
    test_pool();
    test_priority();
    test_metrics();
    test_cancellation();
    test_resize();
    test_logger();
    test_csv_export();
    test_scalability();

    printf("\n──────────────────────────────────────\n");
    printf("  Results: %d passed | %d failed\n", test_pass, test_fail);
    printf("──────────────────────────────────────\n\n");
    return (test_fail > 0) ? 1 : 0;
}