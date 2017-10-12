#ifdef CONFIG_PTH
#ifndef QEMU_THREAD_PTH_H
#define QEMU_THREAD_PTH_H

#include "util/coroutine-ucontext-pth.h"
#include "qemu/thread.h"
#include <pth.h>
#include <semaphore.h>
#include "qemu/thread-pth-internal.h"

typedef QemuMutex QemuRecMutex;
#define qemu_rec_mutex_destroy qemu_mutex_destroy
#define qemu_rec_mutex_lock qemu_mutex_lock
#define qemu_rec_mutex_try_lock qemu_mutex_try_lock
#define qemu_rec_mutex_unlock qemu_mutex_unlock

typedef struct AioHandler AioHandler;

struct rcu_reader_data {
    /* Data used by both reader and synchronize_rcu() */
    unsigned long ctr;
    bool waiting;

    /* Data used by reader only */
    unsigned depth;

    /* Data used for registry, protected by rcu_registry_lock */
    QLIST_ENTRY(rcu_reader_data) node;
};

typedef struct IOThread IOThread;
typedef struct pth_wrapper
{
    pth_t pth_thread;

// POSIX ALTERNATIVE TLS

    bool iothread_locked;

    // aio
    GPollFD *pollfds;
    AioHandler **nodes;
    unsigned npfd;
    unsigned  nalloc;
    Notifier  pollfds_cleanup_notifier;
    bool aio_init;

    // current cpu
    CPUState *current_cpu;

    // coroutine
    QSLIST_HEAD(, Coroutine) alloc_pool;
    unsigned int alloc_pool_size;
    Notifier coroutine_pool_cleanup_notifier;
    CoroutineUContext leader;
    Coroutine *current;

    //iothread
    IOThread *my_iothread;

    // rcu
    struct rcu_reader_data rcu_reader;

    // translation block context
    int have_tb_lock;

    char* thread_name;
}pth_wrapper;

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

struct QemuMutex {
    pthpthread_mutex_t lock;
};

struct QemuCond {
    pthpthread_cond_t cond;
};

struct QemuSemaphore {
    pthpthread_mutex_t lock;
    pthpthread_cond_t cond;
    unsigned int count;
};

struct QemuEvent {
    pthpthread_mutex_t lock;
    pthpthread_cond_t cond;
    unsigned value;
};

struct QemuThread {
    pth_wrapper wrapper;
};

typedef struct threadlist
{
    QemuThread * qemuthread;
    QLIST_ENTRY(threadlist) next;
}threadlist;

pth_wrapper* pth_get_wrapper(void);
void initMainThread(void);


#endif // QEMU_THREAD_PTH_H
#endif // CONFIG_PTH
