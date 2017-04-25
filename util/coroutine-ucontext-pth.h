#ifndef COROUTINE_UCONTEXT_PTH_H
#define COROUTINE_UCONTEXT_PTH_H

#include "qemu/osdep.h"
#include <ucontext.h>
#include "qemu-common.h"
#include "qemu/coroutine_int.h"
#include "qemu/thread.h"

#ifdef CONFIG_PTH

#ifdef CONFIG_VALGRIND_H
#include <valgrind/valgrind.h>
#endif

typedef struct CoroutineUContext{
    Coroutine base;
    void *stack;
    size_t stack_size;
    sigjmp_buf env;

#ifdef CONFIG_VALGRIND_H
    unsigned int valgrind_stack_id;
#endif

} CoroutineUContext;
#endif

#endif
