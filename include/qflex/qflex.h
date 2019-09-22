#ifndef QFLEX_H
#define QFLEX_H

#include <stdbool.h>

#include "qemu/osdep.h"
#include "qemu/thread.h"

#include "qflex/qflex-log.h"
#include "qflex/qflex-arch.h"

#define QFLEX_EXEC_IN  (0)
#define QFLEX_EXEC_OUT (1)

#define QFLEX(name)       glue(qflex_, name)
#define GEN_HELPER(func)  glue(gen_helper_, func)

/** NOTE for cpu_exec (accel/tcg/cpu_exec.c)
  * Depending on the desired type of execution,
  * cpu_exec should break from the double while loop
  * in the correct manner.
  */
typedef enum {
    PROLOGUE,   // Breaks when the Arch State is back to the initial user program
    SINGLESTEP, // Breaks when a single TB (instruction) is executed
    EXCEPTION,   // Breaks when the exeception routine is done
    QEMU        // Normal qemu execution
} QFlexExecType_t;

typedef struct qflex_state_t {
    bool inst_done;
    bool broke_loop;
    bool prologue_done;
    uint64_t prologue_pc;
    QFlexExecType_t exec_type;
    bool profile_enable; // Enables the TCG Helper generation
    bool profiling; // Gather statistic with the executed TCG's
    bool flexus_control;
} qflex_state_t;

typedef struct qflex_pth_t {
    int iexit;
    int iloop;
} qflex_pth_t;

extern qflex_state_t qflexState;
extern qflex_pth_t qflexPth;

/** qflex_api_values_init
 * Inits extern flags and vals
 */
void qflex_api_values_init(CPUState *cpu);
void qflex_configure(QemuOpts *opts, Error **errp);

/** qflex_prologue
 * When starting from a saved vm state, QEMU first batch of instructions
 * are many nested interrupts.
 * This functions skips this part till QEMU is back into the initial USER program.
 * WARNING: If the user program in qemu is interrupted before the prologue is done (such
 *    as with CTRL-C), the prologue will never return.
 */
int qflex_prologue(CPUState *cpu);

/** qflex_singlestep
 * This functions executes a single instruction (as TB) before returning.
 * It sets the broke_loop flag if it broke the main execution loop (in cpu-exec.c)
 */
int qflex_singlestep(CPUState *cpu);

/** qflex_exception
 * This function returns when CPUState is back to EL0.
 */
int qflex_exception(CPUState *cpu);

/** qflex_cpu_step (cpus.c)
 */
int qflex_cpu_step(CPUState *cpu);

/** qflex_cpu_exec (accel/tcg/cpu-exec.c)
 * mirror cpu_exec, with qflex execution flow control
 * for TCG execution. Type defines how the while loop break.
 */
int qflex_cpu_exec(CPUState *cpu);


/* Get and Setters for state flags and vars
 */
static inline qflex_state_t qflex_get_state(void)   { return qflexState; }
static inline bool qflex_is_inst_done(void)         { return qflexState.inst_done; }
static inline bool qflex_is_broke_loop(void)        { return qflexState.broke_loop; }
static inline bool qflex_is_prologue_done(void)     { return qflexState.prologue_done; }
static inline QFlexExecType_t qflex_is_type(void)   { return qflexState.exec_type; }
static inline bool qflex_is_profile_enabled(void)   { return qflexState.profile_enable; }
static inline bool qflex_is_profiling(void)         { return qflexState.profiling; }
static inline bool qflex_is_flexus_control(void)    { return qflexState.flexus_control; }

static inline void qflex_update_inst_done(bool done) {
    qflexState.inst_done = done; }
static inline void qflex_update_broke_loop(bool broke) {
    qflexState.broke_loop = broke; }
static inline void qflex_update_prologue_done(uint64_t cur_pc) {
    qflexState.prologue_done = (cur_pc == qflexState.prologue_pc); }
static inline void qflex_update_exec_type(QFlexExecType_t type) {
    qflexState.exec_type = type; }
static inline void qflex_update_profiling(bool profile) {
    qflexState.profiling = profile; }
static inline void qflex_update_flexus_control(bool control) {
    qflexState.flexus_control = control; }


static inline qflex_pth_t qflex_get_pth(void) { return qflexPth; }
static inline void qflex_pth_loop_init(void) {
    qflexPth.iexit = 0;
}
static inline bool qflex_pth_loop_check_done(void) {
    if(qflexPth.iloop > 0) {
        if(++qflexPth.iexit > qflexPth.iloop) {
            qflexPth.iexit = 0;
            return true;
        }
    }
    return false;
}

#endif /* QFLEX_H */
