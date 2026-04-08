#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_QUEUE_SIZE 2048

typedef struct {
    void  (*function)(void*);
    void*   arg;
    int     task_id;
} Task;

typedef struct {
    pthread_t*      threads;
    Task*           queue;
    int             pool_size;
    int             queue_front;
    int             queue_rear;
    int             queue_count;
    int             active_count;
    int             tasks_completed;
    int             stop;
    pthread_mutex_t lock;
    pthread_cond_t  notify;
} ThreadPool;

ThreadPool* pool_init(int size);
int         task_submit(ThreadPool* pool, void (*func)(void*), void* arg, int task_id);
void        pool_stats(ThreadPool* pool);
void        pool_wait_all(ThreadPool* pool);
void        pool_destroy(ThreadPool* pool);

#endif