#include "qemu/osdep.h"
#include "cpu.h"
#include "exec/helper-proto.h"
#include "exec/log.h"

#include "qflex/qflex.h"
#include "qflex/qflex-profiling.h"
#include "qflex/qflex-helper.h"

/* TCG helper functions. (See exec/helper-proto.h  and target/arch/helper.h)
 * This one expands prototypes for the helper functions.
 * They get executed in the TB
 * To use them: in translate.c or translate-a64.c
 * ex: HELPER(qflex_func)(arg1, arg2, ..., argn)
 * gen_helper_qflex_func(arg1, arg2, ..., argn)
 */

/**
 * @brief HELPER(qflex_executed_instruction)
 * location: location of the gen_helper_ in the transalation.
 *           EXEC_IN : Started executing a TB
 *           EXEC_OUT: Done executing a TB, NOTE: Branches don't trigger this helper.
 */
void HELPER(qflex_executed_instruction)(CPUARMState* env, uint64_t pc, int flags, int location) {
    CPUState *cs = CPU(arm_env_get_cpu(env));

    switch(location) {
    case QFLEX_EXEC_IN:
        if(unlikely(qflex_loglevel_mask(QFLEX_LOG_TB_EXEC)) && // To not print twice, Profile has priority
                unlikely(!qflex_loglevel_mask(QFLEX_LOG_PROFILE))) {
            qemu_log_lock();
            qemu_log("IN[%d]  :", cs->cpu_index);
            log_target_disas(cs, pc, 4, flags);
            qemu_log_unlock();
        }
        qflex_update_inst_done(true);
        break;
    default: break;
    }
}

void HELPER(qflex_profile)(CPUARMState* env, uint64_t pc, int flags, int l1h, int l2h) {
    if(!qflex_is_profiling()) return; // If not profiling, don't count statistics

    int el = (arm_current_el(env) == 0) ? 0 : 1;
    qflex_insn_profiles_l1h[el][l1h]++;
    qflex_insn_profiles_l2h[el][l1h][l2h]++;

    if(unlikely(qflex_loglevel_mask(QFLEX_LOG_PROFILE))) {
        CPUState *cs = CPU(arm_env_get_cpu(env));
        const char* l1h_str = qflex_profile_get_char_l1h(l1h);
        const char* l2h_str = qflex_profile_get_char_l2h(l1h, l2h);

        char profile[256];
        snprintf(profile, 256, "QFLEX_LOG_PROFILE:%02i:%02i|%14s:%-18s | ", l1h, l2h, l1h_str, l2h_str);
        qemu_log_lock();
        qemu_log("%s", profile);
        log_target_disas(cs, pc, 4, flags);
        qemu_log_unlock();
    }
}

/**
 * @brief HELPER(qflex_magic_insn)
 * In ARM, hint instruction (which is like a NOP) comes with an int with range 0-127
 * Big part of this range is defined as a normal NOP.
 * Too see which indexes are already used ref (curently 39-127 is free) :
 * https://developer.arm.com/docs/ddi0596/a/a64-base-instructions-alphabetic-order/hint-hint-instruction
 *
 * This function is called when a HINT n (90 < n < 127) TB is executed
 * nop_op: in HINT n, it's the selected n.
 *
 */
void HELPER(qflex_magic_insn)(int nop_op) {
    switch(nop_op) {
    case 100: qflex_log_mask_enable(QFLEX_LOG_INTERRUPT); break;
    case 101: qflex_log_mask_disable(QFLEX_LOG_INTERRUPT); break;
    case 102: qflex_log_mask_enable(QFLEX_LOG_MAGIC_INSN); break;
    case 103: qflex_log_mask_disable(QFLEX_LOG_MAGIC_INSN); break;
    default: break;
    }
    qflex_log_mask(QFLEX_LOG_MAGIC_INSN,"MAGIC_INST:%u\n", nop_op);
}

/**
 * @brief HELPER(qflex_exception_return)
 * This helper gets called after a ERET TB execution is done.
 * The env passed as argument has already changed EL and jumped to the ELR.
 * For the moment not needed.
 */
void HELPER(qflex_exception_return)(CPUARMState *env) { return; }

