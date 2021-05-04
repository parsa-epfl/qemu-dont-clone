//  DO-NOT-REMOVE begin-copyright-block
// QFlex consists of several software components that are governed by various
// licensing terms, in addition to software that was developed internally.
// Anyone interested in using QFlex needs to fully understand and abide by the
// licenses governing all the software components.
// 
// ### Software developed externally (not by the QFlex group)
// 
//     * [NS-3] (https://www.gnu.org/copyleft/gpl.html)
//     * [QEMU] (http://wiki.qemu.org/License)
//     * [SimFlex] (http://parsa.epfl.ch/simflex/)
//     * [GNU PTH] (https://www.gnu.org/software/pth/)
// 
// ### Software developed internally (by the QFlex group)
// **QFlex License**
// 
// QFlex
// Copyright (c) 2020, Parallel Systems Architecture Lab, EPFL
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
// 
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright notice,
//       this list of conditions and the following disclaimer in the documentation
//       and/or other materials provided with the distribution.
//     * Neither the name of the Parallel Systems Architecture Laboratory, EPFL,
//       nor the names of its contributors may be used to endorse or promote
//       products derived from this software without specific prior written
//       permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE PARALLEL SYSTEMS ARCHITECTURE LABORATORY,
// EPFL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//  DO-NOT-REMOVE end-copyright-block
#ifndef QEMU_THREAD_H
#define QEMU_THREAD_H

#include "qemu/processor.h"
#include "qemu/atomic.h"

typedef struct QemuMutex QemuMutex;
typedef struct QemuCond QemuCond;
typedef struct QemuSemaphore QemuSemaphore;
typedef struct QemuEvent QemuEvent;
typedef struct QemuLockCnt QemuLockCnt;
typedef struct QemuThread QemuThread;

#ifdef _WIN32
#include "qemu/thread-win32.h"
#else
        #ifndef CONFIG_PTH
            #include "qemu/thread-posix.h"
        #else
            #include "qemu/thread-pth.h"
            #include <pth.h>
            #include "qemu/thread-pth-internal.h"
            /* general success return value */
                #ifdef OK
                    #undef OK
                #endif
            #define OK 0
            #define pth_error(a,b) (-1)
        #endif

#endif


#ifdef CONFIG_PTH
	#define PTH_UPDATE_CONTEXT \
        	pth_wrapper* w = pth_get_wrapper();

	#define PTH(NAME) w->NAME
	#define PTH_X(NAME, ORIG) w->NAME
	#define PTH_YIELD pth_yield(NULL);
	#define PTH_TLS_GET(NAME) pth_get_wrapper()->NAME
#else
	#define PTH(NAME) NAME
	#define PTH_UPDATE_CONTEXT
	#define PTH_X(NAME, ORIG) ORIG
	#define PTH_YIELD
    #define PTH_TLS_GET(NAME) assert(false);
#endif

#define QEMU_THREAD_JOINABLE 0
#define QEMU_THREAD_DETACHED 1

void qemu_mutex_init(QemuMutex *mutex);
void qemu_mutex_destroy(QemuMutex *mutex);
void qemu_mutex_lock(QemuMutex *mutex);
int qemu_mutex_trylock(QemuMutex *mutex);
void qemu_mutex_unlock(QemuMutex *mutex);

/* Prototypes for other functions are in thread-posix.h/thread-win32.h.  */
void qemu_rec_mutex_init(QemuRecMutex *mutex);

void qemu_cond_init(QemuCond *cond);
void qemu_cond_destroy(QemuCond *cond);

/*
 * IMPORTANT: The implementation does not guarantee that pthread_cond_signal
 * and pthread_cond_broadcast can be called except while the same mutex is
 * held as in the corresponding pthread_cond_wait calls!
 */
void qemu_cond_signal(QemuCond *cond);
void qemu_cond_broadcast(QemuCond *cond);
void qemu_cond_wait(QemuCond *cond, QemuMutex *mutex);

void qemu_sem_init(QemuSemaphore *sem, int init);
void qemu_sem_post(QemuSemaphore *sem);
void qemu_sem_wait(QemuSemaphore *sem);
int qemu_sem_timedwait(QemuSemaphore *sem, int ms);
void qemu_sem_destroy(QemuSemaphore *sem);

void qemu_event_init(QemuEvent *ev, bool init);
void qemu_event_set(QemuEvent *ev);
void qemu_event_reset(QemuEvent *ev);
void qemu_event_wait(QemuEvent *ev);
void qemu_event_destroy(QemuEvent *ev);

void qemu_thread_create(QemuThread *thread, const char *name,
                        void *(*start_routine)(void *),
                        void *arg, int mode);
void *qemu_thread_join(QemuThread *thread);
void qemu_thread_get_self(QemuThread *thread);
bool qemu_thread_is_self(QemuThread *thread);
void qemu_thread_exit(void *retval);
void qemu_thread_naming(bool enable);

struct Notifier;
void qemu_thread_atexit_add(struct Notifier *notifier);
void qemu_thread_atexit_remove(struct Notifier *notifier);

typedef struct QemuSpin {
    int value;
} QemuSpin;

static inline void qemu_spin_init(QemuSpin *spin)
{
    __sync_lock_release(&spin->value);
}

static inline void qemu_spin_lock(QemuSpin *spin)
{
    while (unlikely(__sync_lock_test_and_set(&spin->value, true))) {
        while (atomic_read(&spin->value)) {
            cpu_relax();
        }
    }
}

static inline bool qemu_spin_trylock(QemuSpin *spin)
{
    return __sync_lock_test_and_set(&spin->value, true);
}

static inline bool qemu_spin_locked(QemuSpin *spin)
{
    return atomic_read(&spin->value);
}

static inline void qemu_spin_unlock(QemuSpin *spin)
{
    __sync_lock_release(&spin->value);
}

struct QemuLockCnt {
#ifndef CONFIG_LINUX
    QemuMutex mutex;
#endif
    unsigned count;
};

/**
 * qemu_lockcnt_init: initialize a QemuLockcnt
 * @lockcnt: the lockcnt to initialize
 *
 * Initialize lockcnt's counter to zero and prepare its mutex
 * for usage.
 */
void qemu_lockcnt_init(QemuLockCnt *lockcnt);

/**
 * qemu_lockcnt_destroy: destroy a QemuLockcnt
 * @lockcnt: the lockcnt to destruct
 *
 * Destroy lockcnt's mutex.
 */
void qemu_lockcnt_destroy(QemuLockCnt *lockcnt);

/**
 * qemu_lockcnt_inc: increment a QemuLockCnt's counter
 * @lockcnt: the lockcnt to operate on
 *
 * If the lockcnt's count is zero, wait for critical sections
 * to finish and increment lockcnt's count to 1.  If the count
 * is not zero, just increment it.
 *
 * Because this function can wait on the mutex, it must not be
 * called while the lockcnt's mutex is held by the current thread.
 * For the same reason, qemu_lockcnt_inc can also contribute to
 * AB-BA deadlocks.  This is a sample deadlock scenario:
 *
 *            thread 1                      thread 2
 *            -------------------------------------------------------
 *            qemu_lockcnt_lock(&lc1);
 *                                          qemu_lockcnt_lock(&lc2);
 *            qemu_lockcnt_inc(&lc2);
 *                                          qemu_lockcnt_inc(&lc1);
 */
void qemu_lockcnt_inc(QemuLockCnt *lockcnt);

/**
 * qemu_lockcnt_dec: decrement a QemuLockCnt's counter
 * @lockcnt: the lockcnt to operate on
 */
void qemu_lockcnt_dec(QemuLockCnt *lockcnt);

/**
 * qemu_lockcnt_dec_and_lock: decrement a QemuLockCnt's counter and
 * possibly lock it.
 * @lockcnt: the lockcnt to operate on
 *
 * Decrement lockcnt's count.  If the new count is zero, lock
 * the mutex and return true.  Otherwise, return false.
 */
bool qemu_lockcnt_dec_and_lock(QemuLockCnt *lockcnt);

/**
 * qemu_lockcnt_dec_if_lock: possibly decrement a QemuLockCnt's counter and
 * lock it.
 * @lockcnt: the lockcnt to operate on
 *
 * If the count is 1, decrement the count to zero, lock
 * the mutex and return true.  Otherwise, return false.
 */
bool qemu_lockcnt_dec_if_lock(QemuLockCnt *lockcnt);

/**
 * qemu_lockcnt_lock: lock a QemuLockCnt's mutex.
 * @lockcnt: the lockcnt to operate on
 *
 * Remember that concurrent visits are not blocked unless the count is
 * also zero.  You can use qemu_lockcnt_count to check for this inside a
 * critical section.
 */
void qemu_lockcnt_lock(QemuLockCnt *lockcnt);

/**
 * qemu_lockcnt_unlock: release a QemuLockCnt's mutex.
 * @lockcnt: the lockcnt to operate on.
 */
void qemu_lockcnt_unlock(QemuLockCnt *lockcnt);

/**
 * qemu_lockcnt_inc_and_unlock: combined unlock/increment on a QemuLockCnt.
 * @lockcnt: the lockcnt to operate on.
 *
 * This is the same as
 *
 *     qemu_lockcnt_unlock(lockcnt);
 *     qemu_lockcnt_inc(lockcnt);
 *
 * but more efficient.
 */
void qemu_lockcnt_inc_and_unlock(QemuLockCnt *lockcnt);

/**
 * qemu_lockcnt_count: query a LockCnt's count.
 * @lockcnt: the lockcnt to query.
 *
 * Note that the count can change at any time.  Still, while the
 * lockcnt is locked, one can usefully check whether the count
 * is non-zero.
 */
unsigned qemu_lockcnt_count(QemuLockCnt *lockcnt);

#endif
