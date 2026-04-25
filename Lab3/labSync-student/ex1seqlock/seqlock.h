#ifndef SEQLOCK_H
#define SEQLOCK_H

#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>

typedef struct pthread_seqlock {
    _Atomic unsigned seq;
    pthread_mutex_t write_lock;
} pthread_seqlock_t;

typedef struct _pthread_seqlock_readstate {
    const pthread_seqlock_t *lock;
    unsigned begin_seq;
} _pthread_seqlock_readstate_t;

static _Thread_local _pthread_seqlock_readstate_t _seqlock_readstate = {0};

static inline int pthread_seqlock_init(pthread_seqlock_t *seqlock)
{
    if (seqlock == NULL) {
        return EINVAL;
    }

    atomic_init(&seqlock->seq, 0U);
    return pthread_mutex_init(&seqlock->write_lock, NULL);
}

static inline int pthread_seqlock_destroy(pthread_seqlock_t *seqlock)
{
    if (seqlock == NULL) {
        return EINVAL;
    }

    return pthread_mutex_destroy(&seqlock->write_lock);
}

/*
 * README contains `pthread_rwlock_destroy(pthread_seqlock_t*)`;
 * map it to the intended seqlock destroy API for compatibility.
 */
#define pthread_rwlock_destroy pthread_seqlock_destroy

static inline int pthread_seqlock_wrlock(pthread_seqlock_t *seqlock)
{
    int rc;

    if (seqlock == NULL) {
        return EINVAL;
    }

    rc = pthread_mutex_lock(&seqlock->write_lock);
    if (rc != 0) {
        return rc;
    }

    atomic_fetch_add_explicit(&seqlock->seq, 1U, memory_order_acq_rel);
    return 0;
}

static inline int pthread_seqlock_wrunlock(pthread_seqlock_t *seqlock)
{
    if (seqlock == NULL) {
        return EINVAL;
    }

    atomic_fetch_add_explicit(&seqlock->seq, 1U, memory_order_release);
    return pthread_mutex_unlock(&seqlock->write_lock);
}

/*
 * Reader side:
 * - waits while sequence is odd (writer in progress)
 * - stores sequence snapshot in thread-local read state
 */
static inline int pthread_seqlock_rdlock(pthread_seqlock_t *seqlock)
{
    unsigned seq;

    if (seqlock == NULL) {
        return EINVAL;
    }

    do {
        seq = atomic_load_explicit(&seqlock->seq, memory_order_acquire);
        if ((seq & 1U) == 1U) {
            sched_yield();
        }
    } while ((seq & 1U) == 1U);

    _seqlock_readstate.lock = seqlock;
    _seqlock_readstate.begin_seq = seq;
    return 0;
}

/*
 * Returns:
 * - 0 on successful read section (no concurrent writer modified data)
 * - EAGAIN if a write happened while reading, caller should retry read
 */
static inline int pthread_seqlock_rdunlock(pthread_seqlock_t *seqlock)
{
    unsigned begin_seq;
    unsigned end_seq;

    if (seqlock == NULL) {
        return EINVAL;
    }

    if (_seqlock_readstate.lock != seqlock) {
        return EPERM;
    }

    begin_seq = _seqlock_readstate.begin_seq;
    end_seq = atomic_load_explicit(&seqlock->seq, memory_order_acquire);
    _seqlock_readstate.lock = NULL;
    _seqlock_readstate.begin_seq = 0U;

    if ((end_seq & 1U) == 1U || end_seq != begin_seq) {
        return EAGAIN;
    }

    return 0;
}

#endif
