// #ifdef CONFIG_AWS
#include <assert.h>
#include "qflex/qflex.h"
#include "qflex/fa-qflex.h"
#include "qflex/fa-qflex-aws.h"
#include "qflex/fa-qflex-sim.h"


FA_QFlexCmd_short_t* fa_qflex_read_cmd(const char* filename) {
    size_t size;
    FA_QFlexCmd_short_t* cmd = ((FA_QFlexCmd_short_t*) fa_qflex_read_file(filename, &size));
    // assert(size == sizeof(FA_QFlexCmd_short_t));
    return cmd;
}

void fa_qflex_write_cmd(const char* filename, FA_QFlexCmd_short_t in_cmd) {
    qflex_log_mask(QFLEX_LOG_GENERAL, "QEMU: CMD OUT %s in %s\n", "???", filename);
    fa_qflex_write_file(filename, &in_cmd, sizeof(FA_QFlexCmd_short_t));  
}

FA_QFlexCmd_short_t* fa_qflex_cmd_lock_wait(const char *filename) {
    qflex_log_mask(QFLEX_LOG_GENERAL, "QEMU: waiting... ");
    FA_QFlexCmd_short_t* cmd;
    do {
        usleep(1);
        cmd = fa_qflex_read_cmd(filename);
    } while (cmd->cmd == LOCK_WAIT);
    qflex_log_mask(QFLEX_LOG_GENERAL, "QEMU: CMD IN %s\n", "???");
    fa_qflex_write_cmd(filename, cmds_short[LOCK_WAIT]);
    return cmd;
}

void fa_qflex_read_cpu(CPUState *cpu, const char* filename) {
    size_t size;
    void* buffer = fa_qflex_read_file(filename, &size);
    fa_qflex_unpack_archstate(cpu, buffer);
    free(buffer);
}

void fa_qflex_write_cpu(CPUState *cpu, FA_QFlexFile *file) {
    void* buffer = fa_qflex_pack_archstate(cpu);
    fa_qflex_write_file_write(file, buffer); 
    free(buffer);
}

// #endif /* CONFIG_FPGA */
