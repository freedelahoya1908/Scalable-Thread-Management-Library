#include "../include/thread_lifecycle.h"

ThreadHandle* thread_create(void* (*func)(void*), void* arg, int id) {
    ThreadHandle* handle = (ThreadHandle*)malloc(sizeof(ThreadHandle));
    if (!handle) {
        fprintf(stderr, "[ERROR] thread_create: malloc failed\n");
        return NULL;
    }
    handle->id    = id;
    handle->state = THREAD_RUNNING;
    snprintf(handle->task_name, sizeof(handle->task_name), "task_%d", id);

    if (pthread_create(&handle->tid, NULL, func, arg) != 0) {
        fprintf(stderr, "[ERROR] thread_create: pthread_create failed\n");
        free(handle);
        return NULL;
    }
    printf("[Thread %3d] Created   | state: RUNNING\n", id);
    return handle;
}

int thread_join(ThreadHandle* handle) {
    if (!handle) return -1;
    int result = pthread_join(handle->tid, NULL);
    if (result == 0) {
        handle->state = THREAD_TERMINATED;
        printf("[Thread %3d] Joined    | state: TERMINATED\n", handle->id);
    }
    return result;
}

int thread_detach(ThreadHandle* handle) {
    if (!handle) return -1;
    int result = pthread_detach(handle->tid);
    if (result == 0)
        printf("[Thread %3d] Detached  | runs independently\n", handle->id);
    return result;
}

int thread_cancel(ThreadHandle* handle) {
    if (!handle) return -1;
    int result = pthread_cancel(handle->tid);
    if (result == 0) {
        handle->state = THREAD_TERMINATED;
        printf("[Thread %3d] Cancelled | state: TERMINATED\n", handle->id);
    }
    return result;
}

ThreadState thread_status(ThreadHandle* handle) {
    if (!handle) return THREAD_TERMINATED;
    return handle->state;
}

const char* thread_state_str(ThreadState s) {
    switch(s) {
        case THREAD_RUNNING:    return "RUNNING";
        case THREAD_WAITING:    return "WAITING";
        case THREAD_TERMINATED: return "TERMINATED";
        case THREAD_IDLE:       return "IDLE";
        default:                return "UNKNOWN";
    }
}

void thread_free(ThreadHandle* handle) {
    if (handle) free(handle);
}