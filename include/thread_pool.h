#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#define MAX_QUEUE_SIZE     4096
#define MAX_POOL_SIZE      128
#define MAX_TASK_NAME      32
#define MAX_HISTORY_POINTS 512

/* ── Task Priority ── */
typedef enum {
    PRIORITY_NORMAL   = 0,
    PRIORITY_HIGH     = 1,
    PRIORITY_CRITICAL = 2
} TaskPriority;

/* ── Task State ── */
typedef enum {
    TASK_QUEUED    = 0,
    TASK_RUNNING   = 1,
    TASK_COMPLETED = 2,
    TASK_CANCELLED = 3,
    TASK_FAILED    = 4
} TaskState;

/* ── Task ── */
typedef struct {
    void         (*function)(void*);
    void*          arg;
    int            task_id;
    TaskPriority   priority;
    char           name[MAX_TASK_NAME];
    struct timespec submit_time;
    int            cancel_flag;   /* 1 = skip execution */
} Task;

/* ── Worker Thread Info ── */
typedef struct {
    pthread_t   tid;
    int         id;               /* Thread number T01, T02... */
    int         alive;
    int         active;           /* Currently executing a task */
    char        current_task[MAX_TASK_NAME];
    long        tasks_done;
    double      total_exec_ms;
} WorkerInfo;

/* ── Pool Metrics ── */
typedef struct {
    long   submitted;
    long   completed;
    long   failed;
    long   cancelled;
    double avg_wait_ms;
    double avg_exec_ms;
    double max_wait_ms;
    double throughput;
    int    live_threads;
    int    active_threads;
    int    queue_depth;
    double elapsed_sec;
} PoolMetrics;

/* ── Time-series snapshot for charting ── */
typedef struct {
    double elapsed_sec;
    long   completed;
    int    queue_depth;
    int    active_threads;
    int    live_threads;
    double throughput;
    double avg_wait_ms;
} MetricSnapshot;

/* ── Thread Pool ── */
typedef struct {
    WorkerInfo      workers[MAX_POOL_SIZE];
    int             pool_size;
    int             live_count;
    int             target_size;  /* For dynamic scaling */

    /* Task queue — sorted by priority */
    Task            queue[MAX_QUEUE_SIZE];
    int             queue_front;
    int             queue_rear;
    int             queue_count;

    /* Stats */
    long            tasks_submitted;
    long            tasks_completed;
    long            tasks_failed;
    long            tasks_cancelled;
    double          total_wait_ms;
    double          total_exec_ms;
    double          max_wait_ms;

    int             stop;
    int             auto_scale;              /* 1 = enabled */
    int             min_threads;
    int             max_threads;
    pthread_mutex_t lock;
    pthread_cond_t  notify;
    pthread_cond_t  all_done;
    struct timespec start_time;

    /* Time-series history for dashboard */
    MetricSnapshot  history[MAX_HISTORY_POINTS];
    int             history_count;
    pthread_t       monitor_tid;
    int             monitor_running;
} ThreadPool;

/* ── Core API ── */
ThreadPool*  pool_init(int size);
ThreadPool*  pool_init_ex(int size, int min_threads, int max_threads, int auto_scale);
int          task_submit(ThreadPool* pool, void (*func)(void*), void* arg, int task_id);
int          task_submit_priority(ThreadPool* pool, void (*func)(void*), void* arg,
                                  int task_id, TaskPriority priority, const char* name);
int          task_cancel(ThreadPool* pool, int task_id);        /* NEW: cancel queued task by id */
int          pool_resize(ThreadPool* pool, int new_size);       /* NEW: dynamic scaling */
void         pool_stats(ThreadPool* pool);
void         pool_metrics(ThreadPool* pool, PoolMetrics* m);
void         pool_wait_all(ThreadPool* pool);
void         pool_destroy(ThreadPool* pool);

/* ── Metrics & Reporting ── */
int          pool_export_csv(ThreadPool* pool, const char* path);      /* NEW */
int          pool_export_history_json(ThreadPool* pool, const char* path); /* NEW */

const char*  priority_str(TaskPriority p);

#endif
