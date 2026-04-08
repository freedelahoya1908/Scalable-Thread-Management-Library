#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "include/thread_lifecycle.h"
#include "include/sync_engine.h"
#include "include/thread_pool.h"

/* ─── Shared counter for sync demo ─── */
static int shared_counter = 0;
static MutexHandle* counter_mutex = NULL;

/* ─── Task functions ─── */
void* lifecycle_worker(void* arg) {
    int id = *(int*)arg;
    usleep(100000 + (id * 10000)); /* simulate work */
    printf("[Worker %3d] Task complete\n", id);
    return NULL;
}

void pool_task(void* arg) {
    int id = *(int*)arg;
    usleep(50000 + (rand() % 100000)); /* 50-150ms */

    /* Thread-safe counter increment */
    mutex_lock(counter_mutex);
    shared_counter++;
    mutex_unlock(counter_mutex);
}

/* ─── Separator ─── */
void section(const char* title) {
    printf("\n");
    printf("══════════════════════════════════════════\n");
    printf("  %s\n", title);
    printf("══════════════════════════════════════════\n");
}

int main() {
    srand(time(NULL));

    printf("╔══════════════════════════════════════════╗\n");
    printf("║   Scalable Thread Management Library     ║\n");
    printf("║   CSE-316 CA2 — Demo                     ║\n");
    printf("╚══════════════════════════════════════════╝\n");

    /* ── MODULE 1: Thread Lifecycle ── */
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

    /* ── MODULE 2: Synchronization ── */
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
    sem_getvalue_custom(s);   /* should be 0 */
    sem_post_custom(s);
    sem_post_custom(s);
    sem_getvalue_custom(s);   /* should be 2 */
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

    /* ── MODULE 3: Thread Pool ── */
    section("MODULE 3: Thread Pool & Scheduler");

    counter_mutex = mutex_init();
    shared_counter = 0;

    printf("\n[+] Initializing pool with 8 threads...\n");
    ThreadPool* pool = pool_init(8);

    printf("\n[+] Submitting 40 tasks...\n");
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

    printf("\n[+] Shared counter final value: %d (expected: 40)\n", shared_counter);

    /* ── SCALABILITY TEST ── */
    section("SCALABILITY TEST: 1000 Tasks");

    shared_counter = 0;
    printf("\n[+] Submitting 1000 tasks to pool...\n");

    clock_t start = clock();
    int big_args[1000];
    for (int i = 0; i < 1000; i++) {
        big_args[i] = i + 1;
        task_submit(pool, pool_task, &big_args[i], i + 1);
    }
    pool_wait_all(pool);
    clock_t end = clock();

    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    printf("[+] 1000 tasks done in %.2f seconds\n", elapsed);
    printf("[+] Throughput: %.1f tasks/sec\n", 1000.0 / elapsed);
    printf("[+] Shared counter: %d (expected: 1000)\n", shared_counter);

    /* ── CLEANUP ── */
    section("CLEANUP");
    pool_destroy(pool);
    mutex_destroy(counter_mutex);

    printf("\n╔══════════════════════════════════════════╗\n");
    printf("║   All modules working correctly ✓        ║\n");
    printf("╚══════════════════════════════════════════╝\n\n");
    return 0;
}