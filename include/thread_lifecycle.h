#ifndef THREAD_LIFECYCLE_H
#define THREAD_LIFECYCLE_H

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Thread states */
typedef enum {
    THREAD_RUNNING,
    THREAD_WAITING,
    THREAD_TERMINATED,
    THREAD_IDLE
} ThreadState;

/* Thread handle structure */
typedef struct {
    pthread_t   tid;
    ThreadState state;
    int         id;
    char        task_name[64];
} ThreadHandle;

/* API */
ThreadHandle* thread_create(void* (*func)(void*), void* arg, int id);
int           thread_join(ThreadHandle* handle);
int           thread_detach(ThreadHandle* handle);
int           thread_cancel(ThreadHandle* handle);
ThreadState   thread_status(ThreadHandle* handle);
const char*   thread_state_str(ThreadState s);
void          thread_free(ThreadHandle* handle);

#endif