#ifdef CONFIG_SIAVASH
#ifndef __QEMU_THREAD_CONTROLLER_H
#define __QEMU_THREAD_CONTROLLER_H 1

#include "qemu/thread.h"

#define MAX_ACTIVE_THREADS 30
static QemuThread *active_threads[MAX_ACTIVE_THREADS];
static int main_thread = -1;

int add_new_thread(QemuThread *thread);
void remove_thread(int handle);
void set_main_thread(int handle);
void suspend_aThread(int handle);
void resume_aThread(int handle);
void suspend_threads(void);
void resume_threads(void);

#endif
#endif

