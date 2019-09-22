#include "qemu/osdep.h"
#include "cpu.h"
#include "qflex/qflex.h"

/* qflex/qflex-arch.h
 */
#define ENV(cpu) ((CPUARMState *) cpu->env_ptr)
uint64_t QFLEX_GET_ARCH(pc)(CPUState *cs) { return ENV(cs)->pc; }
int      QFLEX_GET_ARCH(el)(CPUState *cs) { return arm_current_el(ENV(cs)); }
