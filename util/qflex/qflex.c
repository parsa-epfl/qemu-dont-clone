#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#include "qemu/osdep.h"
#include "qemu/thread.h"

#include "qapi/error.h"
#include "qemu-common.h"
#include "qemu/error-report.h"
#include "qapi/qmp/types.h"
#include "qapi/qmp/qerror.h"
#include "qemu/option_int.h"
#include "qemu/main-loop.h"

#include "qflex/qflex.h"

qflex_state_t qflexState;
qflex_pth_t qflexPth;

void qflex_api_values_init(CPUState *cpu) {
    qflexState.inst_done = false;
    qflexState.broke_loop = false;
    qflexState.prologue_done = false;
    qflexState.prologue_pc = QFLEX_GET_ARCH(pc)(cpu);
    qflexState.exec_type = QEMU;
    qflexState.profile_enable = false;
    qflexState.profiling = false;
    qflexState.fast_forward = false;

    qflexPth.iloop = 0;
    qflexPth.iexit = 0;
}

void qflex_configure(QemuOpts *opts, Error **errp) {
    qflexState.fast_forward = qemu_opt_get_bool(opts, "ff", false);
    qflexState.profile_enable = qemu_opt_get_bool(opts, "profile", false);
    qflexPth.iloop = qemu_opt_get_number(opts, "pth_iloop", 0);


}

int qflex_prologue(CPUState *cpu) {
    int ret = 0;
    qflex_log_mask(QFLEX_LOG_GENERAL, "QFLEX: PROLOGUE START:%08lx\n"
                   "    -> Skips initial snapshot load long interrupt routine to normal user program\n", QFLEX_GET_ARCH(pc)(cpu));
    qflex_update_exec_type(PROLOGUE);
    while(!qflex_is_prologue_done()) {
        ret = qflex_cpu_step(cpu);
    }
    qflex_log_mask(QFLEX_LOG_GENERAL, "QFLEX: PROLOGUE END  :%08lx\n", QFLEX_GET_ARCH(pc)(cpu));
    qflex_update_inst_done(false);
    return ret;
}

int qflex_singlestep(CPUState *cpu) {
    int ret = 0;
    qflex_update_exec_type(SINGLESTEP);
    while(!qflex_is_inst_done()) {
        ret = qflex_cpu_step(cpu);
    }
    qflex_update_inst_done(false);
    return ret;
}

int qflex_exception(CPUState *cpu) {
    int ret = 0;
    qflex_update_exec_type(EXCEPTION);
    while(QFLEX_GET_ARCH(el)(cpu) != 0) {
        ret = qflex_cpu_step(cpu);
    }
    qflex_update_inst_done(false);
    return ret;
}

#ifdef CONFIG_FLEXUS
#include "../libqflex/api.h"
int advance_qemu(void * obj){
    CPUState *cpu = obj;
    return qflex_singlestep(cpu);
}
#endif

#if !(defined(CONFIG_FLEXUS) || defined(CONFIG_FA_QFLEX))
int qflex_cpu_step(CPUState *cpu) {return 0;}
#endif
