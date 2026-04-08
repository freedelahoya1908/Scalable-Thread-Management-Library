#include "../include/thread_pool.h"

static void* worker_thread(void* arg) {
    ThreadPool* pool = (ThreadPool*)arg;

    while (1) {
        pthread_mutex_lock(&pool->lock);

        /* Wait for work or stop signal */
        while (pool->queue_count == 0 && !pool->stop)
            pthread_cond_wait(&pool->notify, &pool->lock);

        /* Exit if stopped and no work left */
        if (pool->stop && pool->queue_count == 0) {
            pthread_mutex_unlock(&pool->lock);
            pthread_exit(NULL);
        }

        /* Dequeue task */
        Task task = pool->queue[pool->queue_front];
        pool->queue_front = (pool->queue_front + 1) % MAX_QUEUE_SIZE;
        pool->queue_count--;
        pool->active_count++;
        pthread_mutex_unlock(&pool->lock);

        /* Execute task */
        task.function(task.arg);

        /* Mark done */
        pthread_mutex_lock(&pool->lock);
        pool->active_count--;
        pool->tasks_completed++;
        pthread_mutex_unlock(&pool->lock);
    }
    return NULL;
}

ThreadPool* pool_init(int size) {
    if (size <= 0) { fprintf(stderr, "[ERROR] pool_init: invalid size\n"); return NULL; }

    ThreadPool* pool = (ThreadPool*)malloc(sizeof(ThreadPool));
    if (!pool) return NULL;

    pool->threads         = (pthread_t*)malloc(sizeof(pthread_t) * size);
    pool->queue           = (Task*)malloc(sizeof(Task) * MAX_QUEUE_SIZE);
    pool->pool_size       = size;
    pool->queue_front     = 0;
    pool->queue_rear      = 0;
    pool->queue_count     = 0;
    pool->active_count    = 0;
    pool->tasks_completed = 0;
    pool->stop            = 0;

    pthread_mutex_init(&pool->lock, NULL);
    pthread_cond_init(&pool->notify, NULL);

    for (int i = 0; i < size; i++) {
        if (pthread_create(&pool->threads[i], NULL, worker_thread, pool) != 0) {
            fprintf(stderr, "[ERROR] pool_init: failed to create thread %d\n", i);
        }
    }

    printf("[Pool] Initialized | size: %d threads | queue: %d max tasks\n",
           size, MAX_QUEUE_SIZE);
    return pool;
}

int task_submit(ThreadPool* pool, void (*func)(void*), void* arg, int task_id) {
    if (!pool || !func) return -1;

    pthread_mutex_lock(&pool->lock);
    if (pool->queue_count >= MAX_QUEUE_SIZE) {
        fprintf(stderr, "[ERROR] task_submit: queue full!\n");
        pthread_mutex_unlock(&pool->lock);
        return -1;
    }

    pool->queue[pool->queue_rear].function = func;
    pool->queue[pool->queue_rear].arg      = arg;
    pool->queue[pool->queue_rear].task_id  = task_id;
    pool->queue_rear  = (pool->queue_rear + 1) % MAX_QUEUE_SIZE;
    pool->queue_count++;

    pthread_cond_signal(&pool->notify);
    pthread_mutex_unlock(&pool->lock);
    return 0;
}

void pool_stats(ThreadPool* pool) {
    if (!pool) return;
    pthread_mutex_lock(&pool->lock);
    printf("[Pool] Stats | size: %d | active: %d | queued: %d | completed: %d\n",
           pool->pool_size, pool->active_count,
           pool->queue_count, pool->tasks_completed);
    pthread_mutex_unlock(&pool->lock);
}

void pool_wait_all(ThreadPool* pool) {
    if (!pool) return;
    while (1) {
        pthread_mutex_lock(&pool->lock);
        int busy = pool->queue_count + pool->active_count;
        pthread_mutex_unlock(&pool->lock);
        if (busy == 0) break;
        usleep(10000); /* 10ms polling */
    }
}

void pool_destroy(ThreadPool* pool) {
    if (!pool) return;
    pthread_mutex_lock(&pool->lock);
    pool->stop = 1;
    pthread_cond_broadcast(&pool->notify);
    pthread_mutex_unlock(&pool->lock);

    for (int i = 0; i < pool->pool_size; i++)
        pthread_join(pool->threads[i], NULL);

    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->notify);
    free(pool->threads);
    free(pool->queue);
    free(pool);
    printf("[Pool] Destroyed | all threads terminated\n");
}