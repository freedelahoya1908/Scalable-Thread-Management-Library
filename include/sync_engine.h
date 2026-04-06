#ifndef SYNC_ENGINE_H
#define SYNC_ENGINE_H

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

/* Mutex */
typedef struct {
    pthread_mutex_t mutex;
    int             is_locked;
    int             owner_id;
} MutexHandle;

/* Semaphore */
typedef struct {
    sem_t semaphore;
    int   initial_value;
} SemHandle;

/* Barrier */
typedef struct {
    pthread_barrier_t barrier;
    int               count;
} BarrierHandle;

/* Read-Write Lock */
typedef struct {
    pthread_rwlock_t rwlock;
} RWLockHandle;

/* Mutex API */
MutexHandle*   mutex_init();
int            mutex_lock(MutexHandle* m);
int            mutex_trylock(MutexHandle* m);
int            mutex_unlock(MutexHandle* m);
void           mutex_destroy(MutexHandle* m);

/* Semaphore API */
SemHandle*     sem_create(int initial_value);
int            sem_wait_custom(SemHandle* s);
int            sem_post_custom(SemHandle* s);
int            sem_getvalue_custom(SemHandle* s);
void           sem_destroy_custom(SemHandle* s);

/* Barrier API */
BarrierHandle* barrier_init(int count);
int            barrier_wait_custom(BarrierHandle* b);
void           barrier_destroy(BarrierHandle* b);

/* RWLock API */
RWLockHandle*  rwlock_init();
int            rwlock_rdlock(RWLockHandle* rw);
int            rwlock_wrlock(RWLockHandle* rw);
int            rwlock_unlock(RWLockHandle* rw);
void           rwlock_destroy(RWLockHandle* rw);

#endif