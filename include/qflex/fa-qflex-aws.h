#include "qemu/osdep.h"
#include "qom/cpu.h"
#include "qflex/fa-qflex-sim.h"


FA_QFlexCmd_short_t* fa_qflex_read_cmd(const char* filename);
void fa_qflex_read_cpu(CPUState *cpu, const char* filename);
void fa_qflex_write_cpu(CPUState *cpu, FA_QFlexFile *file);