#include "../include/sync_engine.h"

/* ───── MUTEX ───── */
MutexHandle* mutex_init() {
    MutexHandle* m = (MutexHandle*)malloc(sizeof(MutexHandle));
    if (!m) return NULL;
    pthread_mutex_init(&m->mutex, NULL);
    m->is_locked = 0;
    m->owner_id  = -1;
    printf("[Mutex]     Initialized\n");
    return m;
}

int mutex_lock(MutexHandle* m) {
    if (!m) return -1;
    int r = pthread_mutex_lock(&m->mutex);
    if (r == 0) { m->is_locked = 1; }
    return r;
}

int mutex_trylock(MutexHandle* m) {
    if (!m) return -1;
    int r = pthread_mutex_trylock(&m->mutex);
    if (r == 0) { m->is_locked = 1; printf("[Mutex]     TryLock succeeded\n"); }
    else         printf("[Mutex]     TryLock failed — already locked\n");
    return r;
}

int mutex_unlock(MutexHandle* m) {
    if (!m) return -1;
    int r = pthread_mutex_unlock(&m->mutex);
    if (r == 0) { m->is_locked = 0; }
    return r;
}

void mutex_destroy(MutexHandle* m) {
    if (!m) return;
    pthread_mutex_destroy(&m->mutex);
    free(m);
    printf("[Mutex]     Destroyed\n");
}

/* ───── SEMAPHORE ───── */
SemHandle* sem_create(int initial_value) {
    SemHandle* s = (SemHandle*)malloc(sizeof(SemHandle));
    if (!s) return NULL;
    sem_init(&s->semaphore, 0, initial_value);
    s->initial_value = initial_value;
    printf("[Semaphore] Created    | initial value: %d\n", initial_value);
    return s;
}

int sem_wait_custom(SemHandle* s) {
    if (!s) return -1;
    int r = sem_wait(&s->semaphore);
    if (r == 0) printf("[Semaphore] Wait       | acquired\n");
    return r;
}

int sem_post_custom(SemHandle* s) {
    if (!s) return -1;
    int r = sem_post(&s->semaphore);
    if (r == 0) printf("[Semaphore] Post       | released\n");
    return r;
}

int sem_getvalue_custom(SemHandle* s) {
    if (!s) return -1;
    int val;
    sem_getvalue(&s->semaphore, &val);
    printf("[Semaphore] Value      | %d\n", val);
    return val;
}

void sem_destroy_custom(SemHandle* s) {
    if (!s) return;
    sem_destroy(&s->semaphore);
    free(s);
    printf("[Semaphore] Destroyed\n");
}

/* ───── BARRIER ───── */
BarrierHandle* barrier_init(int count) {
    BarrierHandle* b = (BarrierHandle*)malloc(sizeof(BarrierHandle));
    if (!b) return NULL;
    pthread_barrier_init(&b->barrier, NULL, count);
    b->count = count;
    printf("[Barrier]   Initialized | count: %d\n", count);
    return b;
}

int barrier_wait_custom(BarrierHandle* b) {
    if (!b) return -1;
    printf("[Barrier]   Waiting...\n");
    int r = pthread_barrier_wait(&b->barrier);
    printf("[Barrier]   Released\n");
    return r;
}

void barrier_destroy(BarrierHandle* b) {
    if (!b) return;
    pthread_barrier_destroy(&b->barrier);
    free(b);
    printf("[Barrier]   Destroyed\n");
}

/* ───── RWLOCK ───── */
RWLockHandle* rwlock_init() {
    RWLockHandle* rw = (RWLockHandle*)malloc(sizeof(RWLockHandle));
    if (!rw) return NULL;
    pthread_rwlock_init(&rw->rwlock, NULL);
    printf("[RWLock]    Initialized\n");
    return rw;
}

int rwlock_rdlock(RWLockHandle* rw) {
    if (!rw) return -1;
    int r = pthread_rwlock_rdlock(&rw->rwlock);
    if (r == 0) printf("[RWLock]    Read lock acquired\n");
    return r;
}

int rwlock_wrlock(RWLockHandle* rw) {
    if (!rw) return -1;
    int r = pthread_rwlock_wrlock(&rw->rwlock);
    if (r == 0) printf("[RWLock]    Write lock acquired\n");
    return r;
}

int rwlock_unlock(RWLockHandle* rw) {
    if (!rw) return -1;
    int r = pthread_rwlock_unlock(&rw->rwlock);
    if (r == 0) printf("[RWLock]    Unlocked\n");
    return r;
}

void rwlock_destroy(RWLockHandle* rw) {
    if (!rw) return;
    pthread_rwlock_destroy(&rw->rwlock);
    free(rw);
    printf("[RWLock]    Destroyed\n");
}