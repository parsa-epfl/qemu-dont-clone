#include "qflex/qflex-api.h"
#include "../libqflex/api.h"

bool fa_qflex_user_mode = false;
bool fa_qflex_running = false;
bool qflex_inst_done = false;
bool qflex_prologue_done = false;
uint64_t qflex_prologue_pc = 0xDEADBEEF;

void qflex_api_values_init(CPUState *cpu) {
    qflex_inst_done = false;
    qflex_prologue_done = false;
    qflex_prologue_pc = cpu_get_program_counter(cpu);
    fa_qflex_user_mode = false;
    fa_qflex_running = false;
}
