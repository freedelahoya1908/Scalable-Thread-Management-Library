#include "../include/thread_pool.h"

#include <errno.h>

const char* priority_str(TaskPriority p) {
    switch(p) {
        case PRIORITY_CRITICAL: return "CRITICAL";
        case PRIORITY_HIGH:     return "HIGH";
        default:                return "NORMAL";
    }
}

static double ms_since(struct timespec* from) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (now.tv_sec  - from->tv_sec)  * 1000.0 +
           (now.tv_nsec - from->tv_nsec) / 1e6;
}

/* ── Forward declarations ── */
static void* worker_thread(void* arg);
static void* monitor_thread(void* arg);
static void  spawn_worker(ThreadPool* pool, int widx);

/* ── Enqueue with priority insertion ── */
static void enqueue_task(ThreadPool* pool, Task* t) {
    pool->queue[pool->queue_rear] = *t;
    int cur = pool->queue_rear;
    pool->queue_rear = (pool->queue_rear + 1) % MAX_QUEUE_SIZE;
    pool->queue_count++;

    /* Bubble high-priority tasks forward */
    int idx = (cur - 1 + MAX_QUEUE_SIZE) % MAX_QUEUE_SIZE;
    int cnt = 1;
    while (cnt < pool->queue_count) {
        if (pool->queue[cur].priority > pool->queue[idx].priority) {
            Task tmp = pool->queue[cur];
            pool->queue[cur] = pool->queue[idx];
            pool->queue[idx] = tmp;
            cur = idx;
            idx = (idx - 1 + MAX_QUEUE_SIZE) % MAX_QUEUE_SIZE;
        } else break;
        cnt++;
    }
}

/* ── Worker Thread ── */
static void* worker_thread(void* arg) {
    void** args     = (void**)arg;
    ThreadPool* pool = (ThreadPool*)args[0];
    int widx         = *(int*)args[1];
    free(args[1]); free(args);

    pool->workers[widx].alive = 1;

    while (1) {
        pthread_mutex_lock(&pool->lock);

        while (pool->queue_count == 0 && !pool->stop)
            pthread_cond_wait(&pool->notify, &pool->lock);

        if (pool->stop && pool->queue_count == 0) {
            pool->workers[widx].alive  = 0;
            pool->workers[widx].active = 0;
            pool->live_count--;
            printf("[Thread %2d] Exiting on shutdown | live=%d\n",
                   pool->workers[widx].id, pool->live_count);
            pthread_cond_broadcast(&pool->all_done);
            pthread_mutex_unlock(&pool->lock);
            break;
        }

        /* Graceful scale-down: if target < current pool size, retire self */
        if (!pool->stop && pool->live_count > pool->target_size &&
            pool->queue_count == 0) {
            pool->workers[widx].alive = 0;
            pool->live_count--;
            printf("[Thread %2d] Scale-down retire | live=%d\n",
                   pool->workers[widx].id, pool->live_count);
            pthread_mutex_unlock(&pool->lock);
            break;
        }

        /* Dequeue */
        Task task = pool->queue[pool->queue_front];
        pool->queue_front = (pool->queue_front + 1) % MAX_QUEUE_SIZE;
        pool->queue_count--;

        /* Check cancellation flag */
        if (task.cancel_flag) {
            pool->tasks_cancelled++;
            printf("[Thread %2d] Skipped  | task_%-4d | CANCELLED\n",
                   pool->workers[widx].id, task.task_id);
            pthread_cond_broadcast(&pool->all_done);
            pthread_mutex_unlock(&pool->lock);
            continue;
        }

        pool->workers[widx].active = 1;
        strncpy(pool->workers[widx].current_task, task.name, MAX_TASK_NAME-1);
        pthread_mutex_unlock(&pool->lock);

        double wait_ms = ms_since(&task.submit_time);

        printf("[Thread %2d] Running  | task_%-4d | %-8s | wait=%.1fms\n",
               pool->workers[widx].id, task.task_id,
               priority_str(task.priority), wait_ms);

        struct timespec exec_start;
        clock_gettime(CLOCK_MONOTONIC, &exec_start);

        task.function(task.arg);

        double exec_ms = ms_since(&exec_start);

        pthread_mutex_lock(&pool->lock);
        pool->tasks_completed++;
        pool->workers[widx].tasks_done++;
        pool->workers[widx].total_exec_ms += exec_ms;
        pool->workers[widx].active = 0;
        pool->workers[widx].current_task[0] = '\0';
        pool->total_wait_ms += wait_ms;
        pool->total_exec_ms += exec_ms;
        if (wait_ms > pool->max_wait_ms) pool->max_wait_ms = wait_ms;

        printf("[Thread %2d] Done     | task_%-4d | exec=%.1fms | completed=%ld\n",
               pool->workers[widx].id, task.task_id,
               exec_ms, pool->tasks_completed);
        pthread_cond_broadcast(&pool->all_done);
        pthread_mutex_unlock(&pool->lock);
    }
    return NULL;
}

/* ── Monitor thread: captures time-series snapshots & auto-scales ── */
static void* monitor_thread(void* arg) {
    ThreadPool* pool = (ThreadPool*)arg;
    while (pool->monitor_running) {
        usleep(250000);  /* 250ms sample rate */

        pthread_mutex_lock(&pool->lock);
        if (!pool->monitor_running) { pthread_mutex_unlock(&pool->lock); break; }

        int active = 0;
        for (int i = 0; i < pool->pool_size; i++)
            if (pool->workers[i].active) active++;

        double elapsed = ms_since(&pool->start_time) / 1000.0;
        double thr = (elapsed > 0) ? pool->tasks_completed / elapsed : 0.0;
        double aw  = (pool->tasks_completed > 0) ?
                     pool->total_wait_ms / pool->tasks_completed : 0.0;

        /* Store snapshot */
        if (pool->history_count < MAX_HISTORY_POINTS) {
            int idx = pool->history_count++;
            pool->history[idx].elapsed_sec    = elapsed;
            pool->history[idx].completed      = pool->tasks_completed;
            pool->history[idx].queue_depth    = pool->queue_count;
            pool->history[idx].active_threads = active;
            pool->history[idx].live_threads   = pool->live_count;
            pool->history[idx].throughput     = thr;
            pool->history[idx].avg_wait_ms    = aw;
        }

        /* Auto-scale logic */
        if (pool->auto_scale) {
            int qd = pool->queue_count;
            int target = pool->target_size;

            if (qd > pool->live_count * 4 && target < pool->max_threads) {
                target = target + 2;
                if (target > pool->max_threads) target = pool->max_threads;
                pool->target_size = target;
                /* Spawn additional workers */
                for (int i = pool->pool_size; i < target && pool->pool_size < MAX_POOL_SIZE; i++) {
                    spawn_worker(pool, pool->pool_size);
                    pool->pool_size++;
                }
                printf("[AutoScale] UP   | queue=%d → target=%d | live=%d\n",
                       qd, target, pool->live_count);
            } else if (qd == 0 && active == 0 && target > pool->min_threads) {
                target = target - 1;
                if (target < pool->min_threads) target = pool->min_threads;
                pool->target_size = target;
                pthread_cond_broadcast(&pool->notify);
                printf("[AutoScale] DOWN | queue=%d → target=%d | live=%d\n",
                       qd, target, pool->live_count);
            }
        }

        pthread_mutex_unlock(&pool->lock);
    }
    return NULL;
}

/* ── Spawn a single worker thread ── */
static void spawn_worker(ThreadPool* pool, int widx) {
    pool->workers[widx].id         = widx + 1;
    pool->workers[widx].alive      = 0;
    pool->workers[widx].active     = 0;
    pool->workers[widx].tasks_done = 0;
    pool->workers[widx].total_exec_ms = 0.0;

    void** args = (void**)malloc(sizeof(void*) * 2);
    int*   p    = (int*)malloc(sizeof(int));
    *p = widx;
    args[0] = pool;
    args[1] = p;

    if (pthread_create(&pool->workers[widx].tid, NULL, worker_thread, args) != 0) {
        fprintf(stderr, "[ERROR] spawn_worker: thread %d create failed\n", widx+1);
        free(p); free(args);
    } else {
        pool->live_count++;
        printf("[Thread %2d] Spawned  | live=%d\n", widx+1, pool->live_count);
    }
}

/* ── pool_init ── */
ThreadPool* pool_init(int size) {
    return pool_init_ex(size, size, size, 0);
}

/* ── pool_init_ex: extended with scaling config ── */
ThreadPool* pool_init_ex(int size, int min_threads, int max_threads, int auto_scale) {
    if (size <= 0 || size > MAX_POOL_SIZE) {
        fprintf(stderr, "[ERROR] pool_init: invalid size %d\n", size);
        return NULL;
    }

    ThreadPool* pool = (ThreadPool*)calloc(1, sizeof(ThreadPool));
    if (!pool) return NULL;

    pool->pool_size   = size;
    pool->target_size = size;
    pool->live_count  = 0;
    pool->queue_front = 0;
    pool->queue_rear  = 0;
    pool->queue_count = 0;
    pool->stop        = 0;
    pool->auto_scale  = auto_scale;
    pool->min_threads = min_threads > 0 ? min_threads : size;
    pool->max_threads = max_threads > 0 ? max_threads : size;
    pool->history_count = 0;

    pthread_mutex_init(&pool->lock, NULL);
    pthread_cond_init(&pool->notify, NULL);
    pthread_cond_init(&pool->all_done, NULL);
    clock_gettime(CLOCK_MONOTONIC, &pool->start_time);

    printf("[Pool] Initializing | size: %d | min: %d | max: %d | auto_scale: %s | queue: %d\n",
           size, pool->min_threads, pool->max_threads,
           auto_scale ? "ON" : "OFF", MAX_QUEUE_SIZE);

    for (int i = 0; i < size; i++) {
        spawn_worker(pool, i);
    }

    /* Start monitor thread */
    pool->monitor_running = 1;
    pthread_create(&pool->monitor_tid, NULL, monitor_thread, pool);

    printf("[Pool] Ready | %d threads alive | queue capacity: %d\n\n",
           pool->live_count, MAX_QUEUE_SIZE);
    return pool;
}

/* ── task_submit (normal priority) ── */
int task_submit(ThreadPool* pool, void (*func)(void*), void* arg, int task_id) {
    return task_submit_priority(pool, func, arg, task_id, PRIORITY_NORMAL, "task");
}

/* ── task_submit_priority ── */
int task_submit_priority(ThreadPool* pool, void (*func)(void*), void* arg,
                          int task_id, TaskPriority priority, const char* name) {
    if (!pool || !func) return -1;

    pthread_mutex_lock(&pool->lock);
    if (pool->queue_count >= MAX_QUEUE_SIZE) {
        fprintf(stderr, "[ERROR] task_submit: queue full!\n");
        pool->tasks_failed++;
        pthread_mutex_unlock(&pool->lock);
        return -1;
    }

    Task t;
    t.function    = func;
    t.arg         = arg;
    t.task_id     = task_id;
    t.priority    = priority;
    t.cancel_flag = 0;
    snprintf(t.name, MAX_TASK_NAME, "%s_%d", name, task_id);
    clock_gettime(CLOCK_MONOTONIC, &t.submit_time);

    pool->tasks_submitted++;
    enqueue_task(pool, &t);

    pthread_cond_signal(&pool->notify);
    pthread_mutex_unlock(&pool->lock);
    return 0;
}

/* ── task_cancel: mark a queued task as cancelled by id ── */
int task_cancel(ThreadPool* pool, int task_id) {
    if (!pool) return -1;
    int cancelled = 0;
    pthread_mutex_lock(&pool->lock);
    for (int i = 0; i < pool->queue_count; i++) {
        int idx = (pool->queue_front + i) % MAX_QUEUE_SIZE;
        if (pool->queue[idx].task_id == task_id && !pool->queue[idx].cancel_flag) {
            pool->queue[idx].cancel_flag = 1;
            cancelled = 1;
            printf("[Pool] Cancellation requested | task_id=%d\n", task_id);
            break;
        }
    }
    pthread_mutex_unlock(&pool->lock);
    return cancelled ? 0 : -1;
}

/* ── pool_resize: dynamically grow/shrink pool ── */
int pool_resize(ThreadPool* pool, int new_size) {
    if (!pool || new_size <= 0 || new_size > MAX_POOL_SIZE) return -1;

    pthread_mutex_lock(&pool->lock);
    int old_size = pool->pool_size;
    pool->target_size = new_size;

    if (new_size > old_size) {
        for (int i = old_size; i < new_size; i++) {
            spawn_worker(pool, i);
        }
        pool->pool_size = new_size;
        printf("[Pool] Resize UP   | %d → %d threads\n", old_size, new_size);
    } else if (new_size < old_size) {
        printf("[Pool] Resize DOWN | %d → %d threads (graceful)\n", old_size, new_size);
        pthread_cond_broadcast(&pool->notify);
    }

    pthread_mutex_unlock(&pool->lock);
    return 0;
}

/* ── pool_stats ── */
void pool_stats(ThreadPool* pool) {
    if (!pool) return;
    pthread_mutex_lock(&pool->lock);

    int active = 0;
    for (int i = 0; i < pool->pool_size; i++)
        if (pool->workers[i].active) active++;

    double avg_wait = pool->tasks_completed > 0 ?
                      pool->total_wait_ms / pool->tasks_completed : 0;
    double avg_exec = pool->tasks_completed > 0 ?
                      pool->total_exec_ms / pool->tasks_completed : 0;

    printf("[Pool] Stats | live=%d | active=%d | queued=%d | completed=%ld | "
           "cancelled=%ld | avg_wait=%.1fms | avg_exec=%.1fms\n",
           pool->live_count, active, pool->queue_count,
           pool->tasks_completed, pool->tasks_cancelled, avg_wait, avg_exec);

    printf("[Pool] Thread breakdown:\n");
    for (int i = 0; i < pool->pool_size; i++) {
        if (!pool->workers[i].alive && pool->workers[i].tasks_done == 0) continue;
        printf("       T%02d: tasks=%ld | avg_exec=%.1fms | %s\n",
               pool->workers[i].id,
               pool->workers[i].tasks_done,
               pool->workers[i].tasks_done > 0 ?
                   pool->workers[i].total_exec_ms / pool->workers[i].tasks_done : 0.0,
               pool->workers[i].active ? "BUSY" :
               pool->workers[i].alive  ? "IDLE" : "RETIRED");
    }
    pthread_mutex_unlock(&pool->lock);
}

/* ── pool_metrics ── */
void pool_metrics(ThreadPool* pool, PoolMetrics* m) {
    if (!pool || !m) return;
    pthread_mutex_lock(&pool->lock);

    int active = 0;
    for (int i = 0; i < pool->pool_size; i++)
        if (pool->workers[i].active) active++;

    m->submitted      = pool->tasks_submitted;
    m->completed      = pool->tasks_completed;
    m->failed         = pool->tasks_failed;
    m->cancelled      = pool->tasks_cancelled;
    m->avg_wait_ms    = pool->tasks_completed > 0 ?
                        pool->total_wait_ms / pool->tasks_completed : 0;
    m->avg_exec_ms    = pool->tasks_completed > 0 ?
                        pool->total_exec_ms / pool->tasks_completed : 0;
    m->max_wait_ms    = pool->max_wait_ms;
    m->live_threads   = pool->live_count;
    m->active_threads = active;
    m->queue_depth    = pool->queue_count;
    m->elapsed_sec    = ms_since(&pool->start_time) / 1000.0;
    m->throughput     = m->elapsed_sec > 0 ?
                        m->completed / m->elapsed_sec : 0;
    pthread_mutex_unlock(&pool->lock);
}

/* ── pool_wait_all ── */
void pool_wait_all(ThreadPool* pool) {
    if (!pool) return;
    pthread_mutex_lock(&pool->lock);
    while (1) {
        int act = 0;
        for (int i = 0; i < pool->pool_size; i++)
            if (pool->workers[i].active) act++;
        if (pool->queue_count == 0 && act == 0) break;
        pthread_cond_wait(&pool->all_done, &pool->lock);
    }
    pthread_mutex_unlock(&pool->lock);
}

/* ── pool_destroy ── */
void pool_destroy(ThreadPool* pool) {
    if (!pool) return;
    printf("[Pool] Shutdown initiated — broadcasting stop signal\n");

    /* Stop monitor first */
    pool->monitor_running = 0;
    pthread_join(pool->monitor_tid, NULL);

    pthread_mutex_lock(&pool->lock);
    pool->stop = 1;
    pthread_cond_broadcast(&pool->notify);
    pthread_mutex_unlock(&pool->lock);

    for (int i = 0; i < pool->pool_size; i++) {
        pthread_join(pool->workers[i].tid, NULL);
    }

    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->notify);
    pthread_cond_destroy(&pool->all_done);
    free(pool);
    printf("[Pool] Destroyed | all threads terminated\n");
}

/* ── Export metrics to CSV ── */
int pool_export_csv(ThreadPool* pool, const char* path) {
    if (!pool || !path) return -1;
    FILE* f = fopen(path, "w");
    if (!f) return -1;

    pthread_mutex_lock(&pool->lock);
    fprintf(f, "elapsed_sec,completed,queue_depth,active_threads,live_threads,throughput,avg_wait_ms\n");
    for (int i = 0; i < pool->history_count; i++) {
        MetricSnapshot* s = &pool->history[i];
        fprintf(f, "%.3f,%ld,%d,%d,%d,%.2f,%.3f\n",
                s->elapsed_sec, s->completed, s->queue_depth,
                s->active_threads, s->live_threads, s->throughput, s->avg_wait_ms);
    }
    pthread_mutex_unlock(&pool->lock);

    fclose(f);
    printf("[Pool] CSV exported → %s (%d snapshots)\n", path, pool->history_count);
    return 0;
}

/* ── Export time-series history as JSON ── */
int pool_export_history_json(ThreadPool* pool, const char* path) {
    if (!pool || !path) return -1;
    FILE* f = fopen(path, "w");
    if (!f) return -1;

    pthread_mutex_lock(&pool->lock);
    fprintf(f, "{\n  \"snapshots\": [\n");
    for (int i = 0; i < pool->history_count; i++) {
        MetricSnapshot* s = &pool->history[i];
        fprintf(f, "    {\"t\":%.3f,\"completed\":%ld,\"queue\":%d,\"active\":%d,"
                   "\"live\":%d,\"throughput\":%.2f,\"avg_wait_ms\":%.3f}%s\n",
                s->elapsed_sec, s->completed, s->queue_depth,
                s->active_threads, s->live_threads, s->throughput, s->avg_wait_ms,
                (i + 1 == pool->history_count) ? "" : ",");
    }
    fprintf(f, "  ]\n}\n");
    pthread_mutex_unlock(&pool->lock);

    fclose(f);
    return 0;
}
