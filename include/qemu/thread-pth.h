#include "config_defines.h"

#ifdef CONFIG_PTH

#ifndef QEMU_THREAD_PTH_H
#define QEMU_THREAD_PTH_H

#include "qemu/thread.h"
#include <semaphore.h>

#include <pth.h>
#include "qemu/thread-pth-internal.h"


typedef QemuMutex QemuRecMutex;
#define qemu_rec_mutex_destroy qemu_mutex_destroy
#define qemu_rec_mutex_lock qemu_mutex_lock
#define qemu_rec_mutex_try_lock qemu_mutex_try_lock
#define qemu_rec_mutex_unlock qemu_mutex_unlock

//#include "qemu/queue.h"
#include "notify.h"
#include "util/coroutine-ucontext-pth.h"
typedef struct AioHandler AioHandler;
typedef struct CPUState CPUState;
typedef struct Coroutine Coroutine;
typedef struct rcu_reader_data rcu_reader_data;
typedef struct IOThread IOThread;

typedef struct CoroutineUContext CoroutineUContext;

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
    CoroutineUContext * leader; // different from original - this is pointer
    Coroutine *current;

    //iothread
    IOThread *my_iothread;

    // rcu
    rcu_reader_data * rcu_reader;// different from original - this is pointer

    // translation block context
    int have_tb_lock;


    // linux user
    CPUState* thread_cpu;
    int mmap_lock_count;


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
    pth_wrapper thread;
};

//Linked List Structure for QemuThread - PTH ONLY
typedef struct threadlist
{
    QemuThread * t;
    struct threadlist * next;
    size_t sz;
}threadlist;

void  get_current_thread(QemuThread** t);
void  get_threadlist_head(threadlist ** h);
void  initThreadList(void);
bool  nodeExists(QemuThread** t);

pth_wrapper* getWrapper(void);


#endif

#endif
