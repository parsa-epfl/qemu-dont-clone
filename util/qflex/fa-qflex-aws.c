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
