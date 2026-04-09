#include <stdio.h>
#include <assert.h>
#include "../include/thread_lifecycle.h"
#include "../include/sync_engine.h"
#include "../include/thread_pool.h"

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
    TEST("thread_create returns non-null", h != NULL);
    TEST("thread state is RUNNING",        h->state == THREAD_RUNNING);

    thread_join(h);
    TEST("thread state after join is TERMINATED", h->state == THREAD_TERMINATED);

    thread_free(h);
    TEST("thread_free does not crash", 1);
}

/* ── Sync tests ── */
void test_sync() {
    printf("\n[Test] Module 2: Synchronization\n");

    MutexHandle* m = mutex_init();
    TEST("mutex_init non-null",     m != NULL);
    TEST("mutex not locked init",   m->is_locked == 0);

    mutex_lock(m);
    TEST("mutex locked after lock", m->is_locked == 1);

    mutex_unlock(m);
    TEST("mutex unlocked after unlock", m->is_locked == 0);
    mutex_destroy(m);

    SemHandle* s = sem_create(3);
    TEST("sem_create non-null", s != NULL);
    int val = sem_getvalue_custom(s);
    TEST("semaphore initial value is 3", val == 3);
    sem_destroy_custom(s);
}

/* ── Pool tests ── */
static int pool_counter = 0;
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
    TEST("pool_init non-null", pool != NULL);
    TEST("pool size is 4",     pool->pool_size == 4);

    pool_mutex   = mutex_init();
    pool_counter = 0;

    int dummy[20];
    for (int i = 0; i < 20; i++) {
        dummy[i] = i;
        task_submit(pool, counter_task, &dummy[i], i);
    }
    pool_wait_all(pool);

    TEST("all 20 tasks completed", pool_counter == 20);
    TEST("tasks_completed counter == 20", pool->tasks_completed == 20);

    mutex_destroy(pool_mutex);
    pool_destroy(pool);
}

/* ── Main ── */
int main() {
    printf("╔══════════════════════════════════════╗\n");
    printf("║   Thread Library — Test Suite        ║\n");
    printf("╚══════════════════════════════════════╝\n");

    test_lifecycle();
    test_sync();
    test_pool();

    printf("\n──────────────────────────────────────\n");
    printf("  Results: %d passed | %d failed\n", test_pass, test_fail);
    printf("──────────────────────────────────────\n\n");

    return (test_fail > 0) ? 1 : 0;
}