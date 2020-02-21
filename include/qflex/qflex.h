#ifndef QFLEX_H
#define QFLEX_H

#include <stdbool.h>

#include "qemu/osdep.h"
#include "qemu/thread.h"

#include "qflex/qflex-log.h"

#define QFLEX_EXEC_IN  (0)
#define QFLEX_EXEC_OUT (1)

/** NOTE for cpu_exec (accel/tcg/cpu_exec.c)
  * Depending on the desired type of execution,
  * cpu_exec should break from the double while loop
  * in the correct manner.
  */
typedef enum {
    PROLOGUE,   // Breaks when the Arch State is back to the initial user program
    SINGLESTEP, // Breaks when a single TB (instruction) is executed
    EXECEXCP,   // Breaks when the exeception routine is done
    QEMU        // Normal qemu execution
} QFlexExecType_t;

extern bool qflex_inst_done;
extern bool qflex_prologue_done;
extern uint64_t qflex_prologue_pc;
extern bool qflex_broke_loop;
extern bool qflex_control_with_flexus;

/** qflex_api_values_init
 * Inits extern flags and vals
 */
void qflex_api_values_init(CPUState *cpu);

/** qflex_prologue
 * When starting from a saved vm state, QEMU first batch of instructions
 * are many nested interrupts.
 * This functions skips this part till QEMU is back into the USER program
 */
int qflex_prologue(CPUState *cpu);
int qflex_singlestep(CPUState *cpu);

/** qflex_cpu_step (cpus.c)
 */
int qflex_cpu_step(CPUState *cpu, QFlexExecType_t type);

/** qflex_cpu_exec (accel/tcg/cpu-exec.c)
 * mirror cpu_exec, with qflex execution flow control
 * for TCG execution. Type defines how the while loop break.
 */
int qflex_cpu_exec(CPUState *cpu, QFlexExecType_t type);


/* Get and Setters for flags and vars
 *
 */
static inline bool qflex_is_inst_done(void)     { return qflex_inst_done; }
static inline bool qflex_is_prologue_done(void) { return qflex_prologue_done; }
static inline bool qflex_update_prologue_done(uint64_t cur_pc) {
    qflex_prologue_done = ((cur_pc >> 48) == 0);
    return qflex_prologue_done;
}
static inline void qflex_update_inst_done(bool done) { qflex_inst_done = done; }

#endif /* QFLEX_H */
