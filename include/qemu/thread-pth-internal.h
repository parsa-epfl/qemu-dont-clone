#include "config_defines.h"
#ifdef CONFIG_PTH

#ifndef QEMU_THREAD_PTH_INTERNAL_H
#define QEMU_THREAD_PTH_INTERNAL_H

#include "qemu/thread.h"
#include <pth.h>
/*
 * Run-time invariant values
 */
#define PTHPTHREAD_DESTRUCTOR_ITERATIONS  4
#define PTHPTHREAD_KEYS_MAX               256
#define PTHPTHREAD_STACK_MIN              8192
#define PTHPTHREAD_STACK_MAX              8388608
#define PTHPTHREAD_THREADS_MAX            10000 /* actually yet no restriction */

/*
 * Flags for threads and thread attributes.
 */
#define PTHPTHREAD_CREATE_DETACHED     0x01
#define PTHPTHREAD_CREATE_JOINABLE     0x02
#define PTHPTHREAD_SCOPE_SYSTEM        0x03
#define PTHPTHREAD_SCOPE_PROCESS       0x04
#define PTHPTHREAD_INHERIT_SCHED       0x05
#define PTHPTHREAD_EXPLICIT_SCHED      0x06

/*
 * Values for cancellation
 */
#define PTHPTHREAD_CANCEL_ENABLE        0x01
#define PTHPTHREAD_CANCEL_DISABLE       0x02
#define PTHPTHREAD_CANCEL_ASYNCHRONOUS  0x01
#define PTHPTHREAD_CANCEL_DEFERRED      0x02
#define PTHPTHREAD_CANCELED             ((void *)-1)

/*
 * Flags for mutex priority attributes
 */
#define PTHPTHREAD_PRIO_INHERIT        0x00
#define PTHPTHREAD_PRIO_NONE           0x01
#define PTHPTHREAD_PRIO_PROTECT        0x02

/*
 * Flags for read/write lock attributes
 */
#define PTHPTHREAD_PROCESS_PRIVATE     0x00
#define PTHPTHREAD_PROCESS_SHARED      0x01

/*
 * Forward structure definitions.
 * These are mostly opaque to the application.
 *
 */
typedef struct  pth_st                      pthpthread_st;
typedef struct  pth_attr_st                      pthpthread_attr_st;
typedef struct  pth_cond_st                      pthpthread_cond_st;
typedef struct  pth_mutex_st                      pthpthread_mutex_st;
typedef struct  pth_rwlock_st                      pthpthread_rwlock_st;
struct sched_param;

/*
 * Primitive system data type definitions required by P1003.1c
 */
typedef struct  pthpthread_st              *pthpthread_t;
typedef struct  pthpthread_attr_st         *pthpthread_attr_t;
typedef int                              pthpthread_key_t;
typedef int                              pthpthread_once_t;
typedef int                              pthpthread_mutexattr_t;
typedef struct  pthpthread_mutex_st        *pthpthread_mutex_t;
typedef int                              pthpthread_condattr_t;
typedef struct  pthpthread_cond_st         *pthpthread_cond_t;
typedef int                              pthpthread_rwlockattr_t;
typedef struct  pthpthread_rwlock_st       *pthpthread_rwlock_t;

/*
 * Once support.
 */
#define PTHPTHREAD_ONCE_INIT 0

/*
 * Mutex static initialization values.
 */
enum pthpthread_mutextype {
    PTHPTHREAD_MUTEX_DEFAULT = 1,
    PTHPTHREAD_MUTEX_RECURSIVE,
    PTHPTHREAD_MUTEX_NORMAL,
    PTHPTHREAD_MUTEX_ERRORCHECK
};

/*
 * Mutex/CondVar/RWLock static initialization values.
 */
#define PTHPTHREAD_MUTEX_INITIALIZER   (pthpthread_mutex_t)(NULL)
#define PTHPTHREAD_COND_INITIALIZER    (pthpthread_cond_t)(NULL)
#define PTHPTHREAD_RWLOCK_INITIALIZER  (pthpthread_rwlock_t)(NULL)
#define          pthpthread_detach(t) __pthpthread_detach(t)

int pthpthread_attr_init(pthpthread_attr_t *attr);
int pthpthread_attr_destroy(pthpthread_attr_t *attr);
int pthpthread_attr_setinheritsched(pthpthread_attr_t *attr, int inheritsched);
int pthpthread_attr_getinheritsched(const pthpthread_attr_t *attr, int *inheritsched);
int pthpthread_attr_setschedparam(pthpthread_attr_t *attr, const struct sched_param *schedparam);
int pthpthread_attr_getschedparam(const pthpthread_attr_t *attr, struct sched_param *schedparam);
int pthpthread_attr_setschedpolicy(pthpthread_attr_t *attr, int schedpolicy);

int __pthpthread_detach(pth_t thread);
int pthpthread_setconcurrency(int new_level);
int pthpthread_abort(pth_t thread);


int pthpthread_attr_getschedpolicy(const pthpthread_attr_t *attr, int *schedpolicy);
int pthpthread_attr_setscope(pthpthread_attr_t *attr, int scope);
int pthpthread_attr_getscope(const pthpthread_attr_t *attr, int *scope);
int pthpthread_attr_setstacksize(pthpthread_attr_t *attr, size_t stacksize);
int pthpthread_attr_getstacksize(const pthpthread_attr_t *attr, size_t *stacksize);
int pthpthread_attr_setstackaddr(pthpthread_attr_t *attr, void *stackaddr);
int pthpthread_attr_getstackaddr(const pthpthread_attr_t *attr, void **stackaddr);
int pthpthread_attr_setdetachstate(pthpthread_attr_t *attr, int detachstate);
int pthpthread_attr_getdetachstate(const pthpthread_attr_t *attr, int *detachstate);
int pthpthread_attr_setguardsize(pthpthread_attr_t *attr, int stacksize);
int pthpthread_attr_getguardsize(const pthpthread_attr_t *attr, int *stacksize);
int pthpthread_attr_setname_np(pthpthread_attr_t *attr, char *name);
int pthpthread_attr_getname_np(const pthpthread_attr_t *attr, char **name);
int pthpthread_attr_setprio_np(pthpthread_attr_t *attr, int prio);
int pthpthread_attr_getprio_np(const pthpthread_attr_t *attr, int *prio);
int pthpthread_create(pth_t *thread, const pthpthread_attr_t *attr, void *(*start_routine)(void *), void *arg);
pth_t pthpthread_self(void);
int pthpthread_equal(pth_t t1, pth_t t2);
int pthpthread_yield_np(void);
void pthpthread_exit(void *value_ptr);
int pthpthread_join(pth_t thread, void **value_ptr);
int pthpthread_once( pthpthread_once_t *once_control, void (*init_routine)(void));
int pthpthread_sigmask(int how, const sigset_t *set, sigset_t *oset);
int pthpthread_kill(pth_t thread, int sig);
int pthpthread_getconcurrency(void);
int pthpthread_key_create(pthpthread_key_t *key, void (*destructor)(void *));
int pthpthread_key_delete(pthpthread_key_t key);
int pthpthread_setspecific(pthpthread_key_t key, const void *value);
void *pthpthread_getspecific(pthpthread_key_t key);
int pthpthread_cancel(pth_t thread);
void pthpthread_testcancel(void);
int pthpthread_setcancelstate(int state, int *oldstate);
int pthpthread_setcanceltype(int type, int *oldtype);
int pthpthread_setschedparam(pth_t pthpthread, int policy, const struct sched_param *param);
int pthpthread_getschedparam(pth_t pthpthread, int *policy, struct sched_param *param);
void pthpthread_cleanup_push(void (*routine)(void *), void *arg);
void pthpthread_cleanup_pop(int execute);
int pthpthread_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void));
int pthpthread_mutexattr_init(pthpthread_mutexattr_t *attr);
int pthpthread_mutexattr_destroy(pthpthread_mutexattr_t *attr);
int pthpthread_mutexattr_setprioceiling(pthpthread_mutexattr_t *attr, int prioceiling);
int pthpthread_mutexattr_getprioceiling(pthpthread_mutexattr_t *attr, int *prioceiling);
int pthpthread_mutexattr_setprotocol(pthpthread_mutexattr_t *attr, int protocol);
int pthpthread_mutexattr_getprotocol(pthpthread_mutexattr_t *attr, int *protocol);
int pthpthread_mutexattr_setpshared(pthpthread_mutexattr_t *attr, int pshared);
int pthpthread_mutexattr_getpshared(pthpthread_mutexattr_t *attr, int *pshared);
int pthpthread_mutexattr_settype(pthpthread_mutexattr_t *attr, int type);
int pthpthread_mutexattr_gettype(pthpthread_mutexattr_t *attr, int *type);
int pthpthread_mutex_init(pthpthread_mutex_t *mutex, const pthpthread_mutexattr_t *attr);
int pthpthread_mutex_destroy(pthpthread_mutex_t *mutex);
int pthpthread_mutex_setprioceiling(pthpthread_mutex_t *mutex, int prioceiling, int *old_ceiling);
int pthpthread_mutex_getprioceiling(pthpthread_mutex_t *mutex, int *prioceiling);
int pthpthread_mutex_lock(pthpthread_mutex_t *mutex);
int pthpthread_mutex_trylock(pthpthread_mutex_t *mutex);
int pthpthread_mutex_unlock(pthpthread_mutex_t *mutex);
int pthpthread_rwlockattr_init(pthpthread_rwlockattr_t *attr);
int pthpthread_rwlockattr_destroy(pthpthread_rwlockattr_t *attr);
int pthpthread_rwlockattr_setpshared(pthpthread_rwlockattr_t *attr, int pshared);
int pthpthread_rwlockattr_getpshared(const pthpthread_rwlockattr_t *attr, int *pshared);
int pthpthread_rwlock_init(pthpthread_rwlock_t *rwlock, const pthpthread_rwlockattr_t *attr);
int pthpthread_rwlock_destroy(pthpthread_rwlock_t *rwlock);
int pthpthread_rwlock_rdlock(pthpthread_rwlock_t *rwlock);
int pthpthread_rwlock_tryrdlock(pthpthread_rwlock_t *rwlock);
int pthpthread_rwlock_wrlock(pthpthread_rwlock_t *rwlock);
int pthpthread_rwlock_trywrlock(pthpthread_rwlock_t *rwlock);
int pthpthread_rwlock_unlock(pthpthread_rwlock_t *rwlock);
int pthpthread_condattr_init(pthpthread_condattr_t *attr);
int pthpthread_condattr_destroy(pthpthread_condattr_t *attr);
int pthpthread_condattr_setpshared(pthpthread_condattr_t *attr, int pshared);
int pthpthread_condattr_getpshared(pthpthread_condattr_t *attr, int *pshared);
int pthpthread_cond_init(pthpthread_cond_t *cond, const pthpthread_condattr_t *attr);
int pthpthread_cond_destroy(pthpthread_cond_t *cond);
int pthpthread_cond_broadcast(pthpthread_cond_t *cond);
int pthpthread_cond_signal(pthpthread_cond_t *cond);
int pthpthread_cond_wait(pthpthread_cond_t *cond, pthpthread_mutex_t *mutex);
int pthpthread_cond_timedwait(pthpthread_cond_t *cond, pthpthread_mutex_t *mutex, const struct timespec *abstime);
#endif

#endif
