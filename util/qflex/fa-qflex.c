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
#include "qflex/fa-qflex.h"
#include "qflex/fa-qflex-sim.h"
#include "qflex/json.h"

FA_QFlexState_t fa_qflexState;

void fa_qflex_configure(QemuOpts *opts, Error **errp) {
    const char *mode;
    fa_qflexState.enabled = qemu_opt_get_bool(opts, "enable", false);

    mode = qemu_opt_get(opts, "mode");
    if(!mode) {
        fa_qflexState.mode = MAGIC;
    } else if (!strcmp(mode, "full")) {
        fa_qflexState.mode = FULL;
    } else if (!strcmp(mode, "magic")) {
        fa_qflexState.mode = MAGIC;
    } else {
        error_setg(errp, "FA-QFLEX: Mode specified '%s' is neither full|magic", mode);
    }

    fa_qflexState.simulate = qemu_opt_get_bool(opts, "sim", false);
    if(!fa_qflexState.enabled && fa_qflexState.simulate) {
        error_setg(errp, "FA-QFLEX: Can't enable simulator if FA-QFLEX is disabled");
    }

    switch(fa_qflexState.mode) {
    case FULL: fa_qflexState.running = true; break;
    case MAGIC: fa_qflexState.running = false; break;
    default: fa_qflexState.running = false; break;
    }

}

static void fa_qflex_write_program_page(CPUState *cpu, const char* filename) {
    uint64_t phys_page, page_size;
    fa_qflex_get_page(cpu, QFLEX_GET_ARCH(pc)(cpu), MMU_INST_FETCH, &phys_page, &page_size);
    fa_qflex_write_file(filename, (void*) phys_page, page_size);
}

static inline int fa_qflex_wait_magic(CPUState *cpu) {
    int ret = 0;
    // FA_QFLEX execution mode switch is nested in QEMU case
    // NOTE: See cpu-exec.c
    qflex_update_exec_type(QEMU);
    while(!fa_qflex_is_running()) {
        ret = qflex_cpu_step(cpu);
    }
    qflex_update_inst_done(false);
    return ret;
}

void fa_qflex_start(CPUState *cpu) {
    int fault;
    pthread_t pid_sim;
    FA_QFlexCmd_t* cmd;
    uint64_t phys_page, page_size;
    qflex_prologue(cpu);
    fprintf(stdout, "Prologue Done\n");

    fa_qflex_writefile_cmd2json(sim_cfg.sim_cmd, cmds[LOCK_WAIT]); // Reset Commands
    fa_qflex_writefile_cmd2json(sim_cfg.qemu_cmd, cmds[LOCK_WAIT]);

    fault = fa_qflex_get_page(cpu, QFLEX_GET_ARCH(pc)(cpu), MMU_INST_FETCH, &phys_page, &page_size);
    assert(!fault);
    sim_cfg.page_size = page_size;

    int inst = 0;
    while(1) {
        if(fa_qflex_is_mode() == MAGIC && !fa_qflex_is_running()) {
            qflex_log_mask(QFLEX_LOG_GENERAL, "Waiting for Magic Instruction.\n");
            qflex_log_mask_disable(QFLEX_LOG_TB_EXEC);
            fa_qflex_writefile_cmd2json(sim_cfg.sim_cmd, cmds[SIM_STOP]);
            pthread_cancel(pid_sim);
            fa_qflex_wait_magic(cpu);
            qflex_log_mask(QFLEX_LOG_GENERAL, "Received Magic Instruction, SIM START.\n");
            pthread_create(&pid_sim, NULL, &fa_qflex_start_sim, &sim_cfg);
        }
        qflex_log_mask_enable(QFLEX_LOG_TB_EXEC);

        //* Communicate with FPGA/SIM
wait_cmd:
        fa_qflex_filewrite_cpu2json(cpu, sim_cfg.qemu_state);
        fa_qflex_write_program_page(cpu, sim_cfg.program_page);
        fa_qflex_writefile_cmd2json(sim_cfg.sim_cmd, cmds[SIM_START]);
        cmd = fa_qflex_cmd2json_lock_wait(sim_cfg.qemu_cmd);

        switch(cmd->cmd) {
        case DATA_LOAD:
        case DATA_STORE:
        case INST_FETCH:
            fault = fa_qflex_get_page(cpu, cmd->addr, cmd->cmd, &phys_page, &page_size);
            if(!fault) {
                fa_qflex_write_file(sim_cfg.program_page, (void*) phys_page, page_size);
                free(cmd);
                goto wait_cmd;
            } else {
                // Fault generated -> Transfer system and step in QEMU
                //fa_qflex_start_transfer(sim_cfg.fpga_cmd);
            }
        case INST_UNDEF:
        case INST_EXCP:
            fa_qflex_fileread_json2cpu(cpu, sim_cfg.sim_state);
            break;
        case CHECK_N_STEP:
            // Don't load new state
            // Step then handle control back to Chisel
            break;
        default:
            fprintf(stderr, "FA-QFLEX: Received unknown command\n");
            exit(1);
            break;
        }
        free(cmd);
        // */

        qflex_singlestep(cpu);
        if(QFLEX_GET_ARCH(el)(cpu) != 0) {
            qflex_exception(cpu);
        }

        inst++;
    }
}

char* fa_qflex_read_file(const char* filename, size_t *size) {
    int ret, fr;
    char *buffer;
    size_t fsize;
    char filepath[PATH_MAX];

    qflex_log_mask(QFLEX_LOG_GENERAL, "READ FILE %s\n", filename);
    snprintf(filepath, PATH_MAX, FA_QFLEX_ROOT_DIR"/%s", filename);

    fr = open(filepath, O_RDONLY, 0666);
    assert(fr);

    lseek(fr, 0, SEEK_END);
    fsize = lseek(fr, 0, SEEK_CUR);
    lseek(fr, 0, SEEK_SET);
    buffer = malloc(fsize + 1);

    ret = pread(fr, buffer, fsize, 0);
    assert(ret);
    close(fr);

    *size = fsize;
    return buffer;
}

int fa_qflex_write_file(const char *filename, void* buffer, size_t size) {
    char filepath[PATH_MAX];
    int fd = -1;
    void *region;
    qflex_log_mask(QFLEX_LOG_GENERAL,
                   "Writing file : "FA_QFLEX_ROOT_DIR"/%s\n", filename);
    if (mkdir(FA_QFLEX_ROOT_DIR, 0777) && errno != EEXIST) {
        qflex_log_mask(QFLEX_LOG_GENERAL,
                       "'mkdir "FA_QFLEX_ROOT_DIR"' failed\n");
        return 1;
    }
    snprintf(filepath, PATH_MAX, FA_QFLEX_ROOT_DIR"/%s", filename);
    if((fd = open(filepath, O_RDWR | O_CREAT | O_TRUNC, 0666)) == -1) {
        qflex_log_mask(QFLEX_LOG_GENERAL,
                       "Program Page dest file: open failed\n"
                       "    filepath:%s\n", filepath);
        return 1;
    }
    if (lseek(fd, size-1, SEEK_SET) == -1) {
        close(fd);
        qflex_log_mask(QFLEX_LOG_GENERAL,
            "Error calling lseek() to 'stretch' the file\n");
        return 1;
    }
    if (write(fd, "", 1) != 1) {
        close(fd);
        qflex_log_mask(QFLEX_LOG_GENERAL,
            "Error writing last byte of the file\n");
        return 1;
    }

    region = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if(region == MAP_FAILED) {
        close(fd);
        qflex_log_mask(QFLEX_LOG_GENERAL,
            "Error dest file: mmap failed");
        return 1;
    }

    memcpy(region, buffer, size);
    msync(region, size, MS_SYNC);
    munmap(region, size);

    close(fd);
    return 0;
}
