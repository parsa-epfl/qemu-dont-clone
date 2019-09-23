#ifndef FA_QFLEX_SIM_H
#define FA_QFLEX_SIM_H

#include "qemu/osdep.h"
#include "qom/cpu.h"

#define SIMULATOR_ROOTDIR "/home/ulises/Projects/armflex"
/* This file contains utilities to communicate between
 * the Chisel3 simulator and QEMU
 */
typedef enum FA_QFlexCmds_t {
    // Commands SIM->QEMU
    DATA_LOAD   = MMU_DATA_LOAD,
    DATA_STORE  = MMU_DATA_STORE,
    INST_FETCH  = MMU_INST_FETCH,
    INST_UNDEF  = 3,
    INST_EXCP   = 4,
    // Commands QEMU->SIM
    SIM_START  = 5, // Load state from QEMU
    SIM_STOP   = 6, //
    // Commands QEMU<->SIM
    LOCK_WAIT   = 7,
    CHECK_N_STEP = 8,
    FA_QFLEXCMDS_NR
} FA_QFlexCmds_t;

typedef struct FA_QFlexCmd_t {
    FA_QFlexCmds_t cmd;
    uint64_t addr;
} FA_QFlexCmd_t;

extern const FA_QFlexCmd_t cmds[FA_QFLEXCMDS_NR];

typedef struct FA_QFlexSimConfig_t {
    /* Names of files where information is shared between sim and QEMU
     */
    const char* sim_state;
    const char* sim_lock;
    const char* sim_cmd;

    const char* qemu_state;
    const char* qemu_lock;
    const char* qemu_cmd;

    const char* program_page;
    size_t page_size;
    /* Path where files are written and read from,
     * it should be something like /dev/shm/qflex
     */
    const char* rootPath;
    /* Path where to launcn simulator from (chisel3-project root),
     */
    const char* simPath;
} FA_QFlexSimConfig_t;

extern FA_QFlexSimConfig_t sim_cfg;

/**
 * @brief fa_qflex_start_sim
 * @param arg  FA_QFlexSimConfig_t cfg - contains all the filenames and paths
 * @return NULL | Start sbt
 */
void* fa_qflex_start_sim(void *arg);

/* (util/fa-qflex-sim.c)
 */
FA_QFlexCmd_t* fa_qflex_loadfile_json2cmd(const char* filename);
void fa_qflex_writefile_cmd2json(const char* filename, FA_QFlexCmd_t in_cmd);

void fa_qflex_fileread_json2cpu(CPUState *cpu, const char* filename);
void fa_qflex_filewrite_cpu2json(CPUState *cpu, const char* filename);

FA_QFlexCmd_t* fa_qflex_cmd2json_lock_wait(const char *filename);

/*
 * These functions are architecture specific so they are in:
 * (target/arch/fa-qflex-helper.c)
 *
 * In case fa_qflexState.simulator is enabled, communication is done
 * with the Chisel3 simulator, and state is serialized and passed
 * through a JSON string.
 */
char* fa_qflex_cpu2json(CPUState *cpu, size_t *size);
void fa_qflex_json2cpu(CPUState *cpu, char* buffer, size_t size);

#endif /* FA_QFLEX_SIM_H */

