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
#include "qemu/thread-pth-internal.h"

/*
**  GLOBAL STUFF
*/

static void pthpthread_shutdown(void)
{
    pth_kill();
    return;
}

static int pthpthread_initialized = FALSE;

#define pthpthread_initialize() \
    do { \
        if (pthpthread_initialized == FALSE) { \
            pthpthread_initialized = TRUE; \
            pth_init(); \
            atexit(pthpthread_shutdown); \
        } \
    } while (0)

/*
**  THREAD ATTRIBUTE ROUTINES
*/

int pthpthread_attr_init(pthpthread_attr_t *attr)
{
    pth_attr_t na;

    pthpthread_initialize();
    if (attr == NULL)
        return pth_error(EINVAL, EINVAL);
    if ((na = pth_attr_new()) == NULL)
        return errno;
    (*attr) = (pthpthread_attr_t)na;
    return OK;
}

int pthpthread_attr_destroy(pthpthread_attr_t *attr)
{
    pth_attr_t na;

    if (attr == NULL || *attr == NULL)
        return pth_error(EINVAL, EINVAL);
    na = (pth_attr_t)(*attr);
    pth_attr_destroy(na);
    *attr = NULL;
    return OK;
}

int pthpthread_attr_setinheritsched(pthpthread_attr_t *attr, int inheritsched)
{
    if (attr == NULL)
        return pth_error(EINVAL, EINVAL);
    /* not supported */
    return pth_error(ENOSYS, ENOSYS);
}

int pthpthread_attr_getinheritsched(const pthpthread_attr_t *attr, int *inheritsched)
{
    if (attr == NULL || inheritsched == NULL)
        return pth_error(EINVAL, EINVAL);
    /* not supported */
    return pth_error(ENOSYS, ENOSYS);
}

int pthpthread_attr_setschedparam(pthpthread_attr_t *attr, const struct sched_param *schedparam)
{
    if (attr == NULL)
        return pth_error(EINVAL, EINVAL);
    /* not supported */
    return pth_error(ENOSYS, ENOSYS);
}

int pthpthread_attr_getschedparam(const pthpthread_attr_t *attr, struct sched_param *schedparam)
{
    if (attr == NULL || schedparam == NULL)
        return pth_error(EINVAL, EINVAL);
    /* not supported */
    return pth_error(ENOSYS, ENOSYS);
}

int pthpthread_attr_setschedpolicy(pthpthread_attr_t *attr, int schedpolicy)
{
    if (attr == NULL)
        return pth_error(EINVAL, EINVAL);
    /* not supported */
    return pth_error(ENOSYS, ENOSYS);
}

int pthpthread_attr_getschedpolicy(const pthpthread_attr_t *attr, int *schedpolicy)
{
    if (attr == NULL || schedpolicy == NULL)
        return pth_error(EINVAL, EINVAL);
    /* not supported */
    return pth_error(ENOSYS, ENOSYS);
}

int pthpthread_attr_setscope(pthpthread_attr_t *attr, int scope)
{
    if (attr == NULL)
        return pth_error(EINVAL, EINVAL);
    /* not supported */
    return pth_error(ENOSYS, ENOSYS);
}

int pthpthread_attr_getscope(const pthpthread_attr_t *attr, int *scope)
{
    if (attr == NULL || scope == NULL)
        return pth_error(EINVAL, EINVAL);
    /* not supported */
    return pth_error(ENOSYS, ENOSYS);
}

int pthpthread_attr_setstacksize(pthpthread_attr_t *attr, size_t stacksize)
{
    if (attr == NULL)
        return pth_error(EINVAL, EINVAL);
    if (!pth_attr_set((pth_attr_t)(*attr), PTH_ATTR_STACK_SIZE, (unsigned int)stacksize))
        return errno;
    return OK;
}

int pthpthread_attr_getstacksize(const pthpthread_attr_t *attr, size_t *stacksize)
{
    if (attr == NULL || stacksize == NULL)
        return pth_error(EINVAL, EINVAL);
    if (!pth_attr_get((pth_attr_t)(*attr), PTH_ATTR_STACK_SIZE, (unsigned int *)stacksize))
        return errno;
    return OK;
}

int pthpthread_attr_setstackaddr(pthpthread_attr_t *attr, void *stackaddr)
{
    if (attr == NULL)
        return pth_error(EINVAL, EINVAL);
    if (!pth_attr_set((pth_attr_t)(*attr), PTH_ATTR_STACK_ADDR, (char *)stackaddr))
        return errno;
    return OK;
}

int pthpthread_attr_getstackaddr(const pthpthread_attr_t *attr, void **stackaddr)
{
    if (attr == NULL || stackaddr == NULL)
        return pth_error(EINVAL, EINVAL);
    if (!pth_attr_get((pth_attr_t)(*attr), PTH_ATTR_STACK_ADDR, (char **)stackaddr))
        return errno;
    return OK;
}

int pthpthread_attr_setdetachstate(pthpthread_attr_t *attr, int detachstate)
{
    int s;

    if (attr == NULL)
        return pth_error(EINVAL, EINVAL);
    if (detachstate == PTHPTHREAD_CREATE_DETACHED)
        s = FALSE;
    else  if (detachstate == PTHPTHREAD_CREATE_JOINABLE)
        s = TRUE;
    else
        return pth_error(EINVAL, EINVAL);
    if (!pth_attr_set((pth_attr_t)(*attr), PTH_ATTR_JOINABLE, s))
        return errno;
    return OK;
}

int pthpthread_attr_getdetachstate(const pthpthread_attr_t *attr, int *detachstate)
{
    int s;

    if (attr == NULL || detachstate == NULL)
        return pth_error(EINVAL, EINVAL);
    if (!pth_attr_get((pth_attr_t)(*attr), PTH_ATTR_JOINABLE, &s))
        return errno;
    if (s == TRUE)
        *detachstate = PTHPTHREAD_CREATE_JOINABLE;
    else
        *detachstate = PTHPTHREAD_CREATE_DETACHED;
    return OK;
}

int pthpthread_attr_setguardsize(pthpthread_attr_t *attr, int stacksize)
{
    if (attr == NULL || stacksize < 0)
        return pth_error(EINVAL, EINVAL);
    /* not supported */
    return pth_error(ENOSYS, ENOSYS);
}

int pthpthread_attr_getguardsize(const pthpthread_attr_t *attr, int *stacksize)
{
    if (attr == NULL || stacksize == NULL)
        return pth_error(EINVAL, EINVAL);
    /* not supported */
    return pth_error(ENOSYS, ENOSYS);
}

int pthpthread_attr_setname_np(pthpthread_attr_t *attr, char *name)
{
    if (attr == NULL || name == NULL)
        return pth_error(EINVAL, EINVAL);
    if (!pth_attr_set((pth_attr_t)(*attr), PTH_ATTR_NAME, name))
        return errno;
    return OK;
}

int pthpthread_attr_getname_np(const pthpthread_attr_t *attr, char **name)
{
    if (attr == NULL || name == NULL)
        return pth_error(EINVAL, EINVAL);
    if (!pth_attr_get((pth_attr_t)(*attr), PTH_ATTR_NAME, name))
        return errno;
    return OK;
}

int pthpthread_attr_setprio_np(pthpthread_attr_t *attr, int prio)
{
    if (attr == NULL || (prio < PTH_PRIO_MIN || prio > PTH_PRIO_MAX))
        return pth_error(EINVAL, EINVAL);
    if (!pth_attr_set((pth_attr_t)(*attr), PTH_ATTR_PRIO, prio))
        return errno;
    return OK;
}

int pthpthread_attr_getprio_np(const pthpthread_attr_t *attr, int *prio)
{
    if (attr == NULL || prio == NULL)
        return pth_error(EINVAL, EINVAL);
    if (!pth_attr_get((pth_attr_t)(*attr), PTH_ATTR_PRIO, prio))
        return errno;
    return OK;
}

/*
**  THREAD ROUTINES
*/
int pthpthread_create(
    pth_t *thread, const pthpthread_attr_t *attr,
    void *(*start_routine)(void *), void *arg)
{
    pth_attr_t na;

    pthpthread_initialize();
    if (thread == NULL || start_routine == NULL)
        return pth_error(EINVAL, EINVAL);
    if (pth_ctrl(PTH_CTRL_GETTHREADS) >= PTHPTHREAD_THREADS_MAX)
        return pth_error(EAGAIN, EAGAIN);
    if (attr != NULL)
        na = (pth_attr_t)(*attr);
    else
        na = PTH_ATTR_DEFAULT;

    *thread = pth_spawn(na, start_routine, arg);



    if (*thread == NULL)
        return pth_error(EAGAIN, EAGAIN);
    return OK;
}

int __pthpthread_detach(pth_t thread)
{
    pth_attr_t na;

    if (thread == NULL)
        return pth_error(EINVAL, EINVAL);
    if ((na = pth_attr_of(thread)) == NULL)
        return errno;
    if (!pth_attr_set(na, PTH_ATTR_JOINABLE, FALSE))
        return errno;
    pth_attr_destroy(na);
    return OK;
}

pth_t pthpthread_self(void)
{
    pthpthread_initialize();
    return pth_self();
}

int pthpthread_equal(pth_t t1, pth_t t2)
{
    return (t1 == t2);
}

int pthpthread_yield_np(void)
{
    pthpthread_initialize();
    pth_yield(NULL);
    return OK;
}

void pthpthread_exit(void *value_ptr)
{
    pthpthread_initialize();
    pth_exit(value_ptr);
    return;
}

int pthpthread_join(pth_t thread, void **value_ptr)
{
    pthpthread_initialize();
    if (!pth_join(thread, value_ptr))
        return errno;
    if (value_ptr != NULL)
        if (*value_ptr == PTH_CANCELED)
            *value_ptr = PTHPTHREAD_CANCELED;
    return OK;
}

int pthpthread_once(
    pthpthread_once_t *once_control, void (*init_routine)(void))
{
    pthpthread_initialize();
    if (once_control == NULL || init_routine == NULL)
        return pth_error(EINVAL, EINVAL);
    if (*once_control != 1)
        init_routine();
    *once_control = 1;
    return OK;
}

int pthpthread_sigmask(int how, const sigset_t *set, sigset_t *oset)
{
    pthpthread_initialize();
    return pth_sigmask(how, set, oset);
}

int pthpthread_kill(pth_t thread, int sig)
{
    if (!pth_raise(thread, sig))
      return errno;
    return OK;
}

/*
**  CONCURRENCY ROUTINES
**
**  We just have to provide the interface, because SUSv2 says:
**  "The pthpthread_setconcurrency() function allows an application to
**  inform the threads implementation of its desired concurrency
**  level, new_level. The actual level of concurrency provided by the
**  implementation as a result of this function call is unspecified."
*/

static int pthpthread_concurrency = 0;

int pthpthread_getconcurrency(void)
{
    return pthpthread_concurrency;
}

int pthpthread_setconcurrency(int new_level)
{
    if (new_level < 0)
        return pth_error(EINVAL, EINVAL);
    pthpthread_concurrency = new_level;
    return OK;
}

/*
**  CONTEXT ROUTINES
*/

int pthpthread_key_create(pthpthread_key_t *key, void (*destructor)(void *))
{
    pthpthread_initialize();
    if (!pth_key_create((pth_key_t *)key, destructor))
        return errno;
    return OK;
}

int pthpthread_key_delete(pthpthread_key_t key)
{
    if (!pth_key_delete((pth_key_t)key))
        return errno;
    return OK;
}

int pthpthread_setspecific(pthpthread_key_t key, const void *value)
{
    if (!pth_key_setdata((pth_key_t)key, value))
        return errno;
    return OK;
}

void *pthpthread_getspecific(pthpthread_key_t key)
{
    return pth_key_getdata((pth_key_t)key);
}

/*
**  CANCEL ROUTINES
*/

int pthpthread_cancel(pth_t thread)
{
    if (!pth_cancel(thread))
        return errno;
    return OK;
}

void pthpthread_testcancel(void)
{
    pth_cancel_point();
    return;
}

int pthpthread_setcancelstate(int state, int *oldstate)
{
    int s, os;

    if (oldstate != NULL) {
        pth_cancel_state(0, &os);
        if (os & PTH_CANCEL_ENABLE)
            *oldstate = PTHPTHREAD_CANCEL_ENABLE;
        else
            *oldstate = PTHPTHREAD_CANCEL_DISABLE;
    }
    if (state != 0) {
        pth_cancel_state(0, &s);
        if (state == PTHPTHREAD_CANCEL_ENABLE) {
            s |= PTH_CANCEL_ENABLE;
            s &= ~(PTH_CANCEL_DISABLE);
        }
        else {
            s |= PTH_CANCEL_DISABLE;
            s &= ~(PTH_CANCEL_ENABLE);
        }
        pth_cancel_state(s, NULL);
    }
    return OK;
}

int pthpthread_setcanceltype(int type, int *oldtype)
{
    int t, ot;

    if (oldtype != NULL) {
        pth_cancel_state(0, &ot);
        if (ot & PTH_CANCEL_DEFERRED)
            *oldtype = PTHPTHREAD_CANCEL_DEFERRED;
        else
            *oldtype = PTHPTHREAD_CANCEL_ASYNCHRONOUS;
    }
    if (type != 0) {
        pth_cancel_state(0, &t);
        if (type == PTHPTHREAD_CANCEL_DEFERRED) {
            t |= PTH_CANCEL_DEFERRED;
            t &= ~(PTH_CANCEL_ASYNCHRONOUS);
        }
        else {
            t |= PTH_CANCEL_ASYNCHRONOUS;
            t &= ~(PTH_CANCEL_DEFERRED);
        }
        pth_cancel_state(t, NULL);
    }
    return OK;
}

/*
**  SCHEDULER ROUTINES
*/

int pthpthread_setschedparam(pth_t pthpthread, int policy, const struct sched_param *param)
{
    /* not supported */
    return pth_error(ENOSYS, ENOSYS);
}

int pthpthread_getschedparam(pth_t pthpthread, int *policy, struct sched_param *param)
{
    /* not supported */
    return pth_error(ENOSYS, ENOSYS);
}

/*
**  CLEANUP ROUTINES
*/

void pthpthread_cleanup_push(void (*routine)(void *), void *arg)
{
    pthpthread_initialize();
    pth_cleanup_push(routine, arg);
    return;
}

void pthpthread_cleanup_pop(int execute)
{
    pth_cleanup_pop(execute);
    return;
}

/*
**  AT-FORK SUPPORT
*/

struct pthpthread_atfork_st {
    void (*prepare)(void);
    void (*parent)(void);
    void (*child)(void);
};
static struct pthpthread_atfork_st pthpthread_atfork_info[PTH_ATFORK_MAX];
static int pthpthread_atfork_idx = 0;

static void pthpthread_atfork_cb_prepare(void *_info)
{
    struct pthpthread_atfork_st *info = (struct pthpthread_atfork_st *)_info;
    info->prepare();
    return;
}
static void pthpthread_atfork_cb_parent(void *_info)
{
    struct pthpthread_atfork_st *info = (struct pthpthread_atfork_st *)_info;
    info->parent();
    return;
}
static void pthpthread_atfork_cb_child(void *_info)
{
    struct pthpthread_atfork_st *info = (struct pthpthread_atfork_st *)_info;
    info->child();
    return;
}

int pthpthread_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void))
{
    struct pthpthread_atfork_st *info;

    if (pthpthread_atfork_idx > PTH_ATFORK_MAX-1)
        return pth_error(ENOMEM, ENOMEM);
    info = &pthpthread_atfork_info[pthpthread_atfork_idx++];
    info->prepare = prepare;
    info->parent  = parent;
    info->child   = child;
    if (!pth_atfork_push(pthpthread_atfork_cb_prepare,
                         pthpthread_atfork_cb_parent,
                         pthpthread_atfork_cb_child, info))
        return errno;
    return OK;
}

/*
**  MUTEX ATTRIBUTE ROUTINES
*/

int pthpthread_mutexattr_init(pthpthread_mutexattr_t *attr)
{
    pthpthread_initialize();
    if (attr == NULL)
        return pth_error(EINVAL, EINVAL);
    /* nothing to do for us */
    return OK;
}

int pthpthread_mutexattr_destroy(pthpthread_mutexattr_t *attr)
{
    if (attr == NULL)
        return pth_error(EINVAL, EINVAL);
    /* nothing to do for us */
    return OK;
}

int pthpthread_mutexattr_setprioceiling(pthpthread_mutexattr_t *attr, int prioceiling)
{
    if (attr == NULL)
        return pth_error(EINVAL, EINVAL);
    /* not supported */
    return pth_error(ENOSYS, ENOSYS);
}

int pthpthread_mutexattr_getprioceiling(pthpthread_mutexattr_t *attr, int *prioceiling)
{
    if (attr == NULL)
        return pth_error(EINVAL, EINVAL);
    /* not supported */
    return pth_error(ENOSYS, ENOSYS);
}

int pthpthread_mutexattr_setprotocol(pthpthread_mutexattr_t *attr, int protocol)
{
    if (attr == NULL)
        return pth_error(EINVAL, EINVAL);
    /* not supported */
    return pth_error(ENOSYS, ENOSYS);
}

int pthpthread_mutexattr_getprotocol(pthpthread_mutexattr_t *attr, int *protocol)
{
    if (attr == NULL)
        return pth_error(EINVAL, EINVAL);
    /* not supported */
    return pth_error(ENOSYS, ENOSYS);
}

int pthpthread_mutexattr_setpshared(pthpthread_mutexattr_t *attr, int pshared)
{
    if (attr == NULL)
        return pth_error(EINVAL, EINVAL);
    /* not supported */
    return pth_error(ENOSYS, ENOSYS);
}

int pthpthread_mutexattr_getpshared(pthpthread_mutexattr_t *attr, int *pshared)
{
    if (attr == NULL)
        return pth_error(EINVAL, EINVAL);
    /* not supported */
    return pth_error(ENOSYS, ENOSYS);
}

int pthpthread_mutexattr_settype(pthpthread_mutexattr_t *attr, int type)
{
    if (attr == NULL)
        return pth_error(EINVAL, EINVAL);
    /* not supported */
    return pth_error(ENOSYS, ENOSYS);
}

int pthpthread_mutexattr_gettype(pthpthread_mutexattr_t *attr, int *type)
{
    if (attr == NULL)
        return pth_error(EINVAL, EINVAL);
    /* not supported */
    return pth_error(ENOSYS, ENOSYS);
}

/*
**  MUTEX ROUTINES
*/

int pthpthread_mutex_init(pthpthread_mutex_t *mutex, const pthpthread_mutexattr_t *attr)
{
    pth_mutex_t *m;

    pthpthread_initialize();
    if (mutex == NULL)
        return pth_error(EINVAL, EINVAL);
    if ((m = (pth_mutex_t *)malloc(sizeof(pth_mutex_t))) == NULL)
        return errno;
    if (!pth_mutex_init(m))
        return errno;
    (*mutex) = (pthpthread_mutex_t)m;
    return OK;
}

int pthpthread_mutex_destroy(pthpthread_mutex_t *mutex)
{
    if (mutex == NULL)
        return pth_error(EINVAL, EINVAL);
    free(*mutex);
    *mutex = NULL;
    return OK;
}

int pthpthread_mutex_setprioceiling(pthpthread_mutex_t *mutex, int prioceiling, int *old_ceiling)
{
    if (mutex == NULL)
        return pth_error(EINVAL, EINVAL);
    if (*mutex == PTHPTHREAD_MUTEX_INITIALIZER)
        if (pthpthread_mutex_init(mutex, NULL) != OK)
            return errno;
    /* not supported */
    return pth_error(ENOSYS, ENOSYS);
}

int pthpthread_mutex_getprioceiling(pthpthread_mutex_t *mutex, int *prioceiling)
{
    if (mutex == NULL)
        return pth_error(EINVAL, EINVAL);
    if (*mutex == PTHPTHREAD_MUTEX_INITIALIZER)
        if (pthpthread_mutex_init(mutex, NULL) != OK)
            return errno;
    /* not supported */
    return pth_error(ENOSYS, ENOSYS);
}

int pthpthread_mutex_lock(pthpthread_mutex_t *mutex)
{
    if (mutex == NULL)
        return pth_error(EINVAL, EINVAL);
    if (*mutex == PTHPTHREAD_MUTEX_INITIALIZER)
        if (pthpthread_mutex_init(mutex, NULL) != OK)
            return errno;
    if (!pth_mutex_acquire((pth_mutex_t *)(*mutex), FALSE, NULL))
        return errno;
    return OK;
}

int pthpthread_mutex_trylock(pthpthread_mutex_t *mutex)
{
    if (mutex == NULL)
        return pth_error(EINVAL, EINVAL);
    if (*mutex == PTHPTHREAD_MUTEX_INITIALIZER)
        if (pthpthread_mutex_init(mutex, NULL) != OK)
            return errno;
    if (!pth_mutex_acquire((pth_mutex_t *)(*mutex), TRUE, NULL))
        return errno;
    return OK;
}

int pthpthread_mutex_unlock(pthpthread_mutex_t *mutex)
{
    if (mutex == NULL)
        return pth_error(EINVAL, EINVAL);
    if (*mutex == PTHPTHREAD_MUTEX_INITIALIZER)
        if (pthpthread_mutex_init(mutex, NULL) != OK)
            return errno;
    if (!pth_mutex_release((pth_mutex_t *)(*mutex)))
        return errno;
    return OK;
}

/*
**  LOCK ATTRIBUTE ROUTINES
*/

int pthpthread_rwlockattr_init(pthpthread_rwlockattr_t *attr)
{
    pthpthread_initialize();
    if (attr == NULL)
        return pth_error(EINVAL, EINVAL);
    /* nothing to do for us */
    return OK;
}

int pthpthread_rwlockattr_destroy(pthpthread_rwlockattr_t *attr)
{
    if (attr == NULL)
        return pth_error(EINVAL, EINVAL);
    /* nothing to do for us */
    return OK;
}

int pthpthread_rwlockattr_setpshared(pthpthread_rwlockattr_t *attr, int pshared)
{
    if (attr == NULL)
        return pth_error(EINVAL, EINVAL);
    /* not supported */
    return pth_error(ENOSYS, ENOSYS);
}

int pthpthread_rwlockattr_getpshared(const pthpthread_rwlockattr_t *attr, int *pshared)
{
    if (attr == NULL)
        return pth_error(EINVAL, EINVAL);
    /* not supported */
    return pth_error(ENOSYS, ENOSYS);
}

/*
**  LOCK ROUTINES
*/

int pthpthread_rwlock_init(pthpthread_rwlock_t *rwlock, const pthpthread_rwlockattr_t *attr)
{
    pth_rwlock_t *rw;

    pthpthread_initialize();
    if (rwlock == NULL)
        return pth_error(EINVAL, EINVAL);
    if ((rw = (pth_rwlock_t *)malloc(sizeof(pth_rwlock_t))) == NULL)
        return errno;
    if (!pth_rwlock_init(rw))
        return errno;
    (*rwlock) = (pthpthread_rwlock_t)rw;
    return OK;
}

int pthpthread_rwlock_destroy(pthpthread_rwlock_t *rwlock)
{
    if (rwlock == NULL)
        return pth_error(EINVAL, EINVAL);
    free(*rwlock);
    *rwlock = NULL;
    return OK;
}

int pthpthread_rwlock_rdlock(pthpthread_rwlock_t *rwlock)
{
    if (rwlock == NULL)
        return pth_error(EINVAL, EINVAL);
    if (*rwlock == PTHPTHREAD_RWLOCK_INITIALIZER)
        if (pthpthread_rwlock_init(rwlock, NULL) != OK)
            return errno;
    if (!pth_rwlock_acquire((pth_rwlock_t *)(*rwlock), PTH_RWLOCK_RD, FALSE, NULL))
        return errno;
    return OK;
}

int pthpthread_rwlock_tryrdlock(pthpthread_rwlock_t *rwlock)
{
    if (rwlock == NULL)
        return pth_error(EINVAL, EINVAL);
    if (*rwlock == PTHPTHREAD_RWLOCK_INITIALIZER)
        if (pthpthread_rwlock_init(rwlock, NULL) != OK)
            return errno;
    if (!pth_rwlock_acquire((pth_rwlock_t *)(*rwlock), PTH_RWLOCK_RD, TRUE, NULL))
        return errno;
    return OK;
}

int pthpthread_rwlock_wrlock(pthpthread_rwlock_t *rwlock)
{
    if (rwlock == NULL)
        return pth_error(EINVAL, EINVAL);
    if (*rwlock == PTHPTHREAD_RWLOCK_INITIALIZER)
        if (pthpthread_rwlock_init(rwlock, NULL) != OK)
            return errno;
    if (!pth_rwlock_acquire((pth_rwlock_t *)(*rwlock), PTH_RWLOCK_RW, FALSE, NULL))
        return errno;
    return OK;
}

int pthpthread_rwlock_trywrlock(pthpthread_rwlock_t *rwlock)
{
    if (rwlock == NULL)
        return pth_error(EINVAL, EINVAL);
    if (*rwlock == PTHPTHREAD_RWLOCK_INITIALIZER)
        if (pthpthread_rwlock_init(rwlock, NULL) != OK)
            return errno;
    if (!pth_rwlock_acquire((pth_rwlock_t *)(*rwlock), PTH_RWLOCK_RW, TRUE, NULL))
        return errno;
    return OK;
}

int pthpthread_rwlock_unlock(pthpthread_rwlock_t *rwlock)
{
    if (rwlock == NULL)
        return pth_error(EINVAL, EINVAL);
    if (*rwlock == PTHPTHREAD_RWLOCK_INITIALIZER)
        if (pthpthread_rwlock_init(rwlock, NULL) != OK)
            return errno;
    if (!pth_rwlock_release((pth_rwlock_t *)(*rwlock)))
        return errno;
    return OK;
}

/*
**  CONDITION ATTRIBUTE ROUTINES
*/

int pthpthread_condattr_init(pthpthread_condattr_t *attr)
{
    pthpthread_initialize();
    if (attr == NULL)
        return pth_error(EINVAL, EINVAL);
    /* nothing to do for us */
    return OK;
}

int pthpthread_condattr_destroy(pthpthread_condattr_t *attr)
{
    if (attr == NULL)
        return pth_error(EINVAL, EINVAL);
    /* nothing to do for us */
    return OK;
}

int pthpthread_condattr_setpshared(pthpthread_condattr_t *attr, int pshared)
{
    if (attr == NULL)
        return pth_error(EINVAL, EINVAL);
    /* not supported */
    return pth_error(ENOSYS, ENOSYS);
}

int pthpthread_condattr_getpshared(pthpthread_condattr_t *attr, int *pshared)
{
    if (attr == NULL)
        return pth_error(EINVAL, EINVAL);
    /* not supported */
    return pth_error(ENOSYS, ENOSYS);
}

/*
**  CONDITION ROUTINES
*/

int pthpthread_cond_init(pthpthread_cond_t *cond, const pthpthread_condattr_t *attr)
{
    pth_cond_t *cn;

    pthpthread_initialize();
    if (cond == NULL)
        return pth_error(EINVAL, EINVAL);
    if ((cn = (pth_cond_t *)malloc(sizeof(pth_cond_t))) == NULL)
        return errno;
    if (!pth_cond_init(cn))
        return errno;
    (*cond) = (pthpthread_cond_t)cn;
    return OK;
}

int pthpthread_cond_destroy(pthpthread_cond_t *cond)
{
    if (cond == NULL)
        return pth_error(EINVAL, EINVAL);
    free(*cond);
    *cond = NULL;
    return OK;
}

int pthpthread_cond_broadcast(pthpthread_cond_t *cond)
{
    if (cond == NULL)
        return pth_error(EINVAL, EINVAL);
    if (*cond == PTHPTHREAD_COND_INITIALIZER)
        if (pthpthread_cond_init(cond, NULL) != OK)
            return errno;
    if (!pth_cond_notify((pth_cond_t *)(*cond), TRUE))
        return errno;
    return OK;
}

int pthpthread_cond_signal(pthpthread_cond_t *cond)
{
    if (cond == NULL)
        return pth_error(EINVAL, EINVAL);
    if (*cond == PTHPTHREAD_COND_INITIALIZER)
        if (pthpthread_cond_init(cond, NULL) != OK)
            return errno;
    if (!pth_cond_notify((pth_cond_t *)(*cond), FALSE))
        return errno;
    return OK;
}

int pthpthread_cond_wait(pthpthread_cond_t *cond, pthpthread_mutex_t *mutex)
{
    if (cond == NULL || mutex == NULL)
        return pth_error(EINVAL, EINVAL);
    if (*cond == PTHPTHREAD_COND_INITIALIZER)
        if (pthpthread_cond_init(cond, NULL) != OK)
            return errno;
    if (*mutex == PTHPTHREAD_MUTEX_INITIALIZER)
        if (pthpthread_mutex_init(mutex, NULL) != OK)
            return errno;
    if (!pth_cond_await((pth_cond_t *)(*cond), (pth_mutex_t *)(*mutex), NULL))
        return errno;
    return OK;
}

int pthpthread_cond_timedwait(pthpthread_cond_t *cond, pthpthread_mutex_t *mutex,
                           const struct timespec *abstime)
{
    pth_event_t ev;
    static pth_key_t ev_key = PTH_KEY_INIT;

    if (cond == NULL || mutex == NULL || abstime == NULL)
        return pth_error(EINVAL, EINVAL);
#ifdef __amigaos__
    if (abstime->ts_sec < 0 || abstime->ts_nsec < 0 || abstime->ts_nsec >= 1000000000)
#else
    if (abstime->tv_sec < 0 || abstime->tv_nsec < 0 || abstime->tv_nsec >= 1000000000)
#endif
        return pth_error(EINVAL, EINVAL);
    if (*cond == PTHPTHREAD_COND_INITIALIZER)
        if (pthpthread_cond_init(cond, NULL) != OK)
            return errno;
    if (*mutex == PTHPTHREAD_MUTEX_INITIALIZER)
        if (pthpthread_mutex_init(mutex, NULL) != OK)
            return errno;
    ev = pth_event(PTH_EVENT_TIME|PTH_MODE_STATIC, &ev_key,
#ifdef __amigaos__
                   pth_time(abstime->ts_sec, (abstime->ts_nsec)/1000)
#else
                   pth_time(abstime->tv_sec, (abstime->tv_nsec)/1000)
#endif
    );
    if (!pth_cond_await((pth_cond_t *)(*cond), (pth_mutex_t *)(*mutex), ev))
        return errno;
    if (pth_event_status(ev) == PTH_STATUS_OCCURRED)
        return ETIMEDOUT;
    return OK;
}

/*
**  POSIX 1003.1j
*/

int pthpthread_abort(pth_t thread)
{
    if (!pth_abort(thread))
        return errno;
    return OK;
}
