#include "qemu/osdep.h"
#include "cpu.h"
#include "exec/helper-proto.h"
#include "exec/log.h"

#include "qflex/qflex-log.h"
#include "qflex/fa-qflex-api.h"

/* TCG helper functions. (See exec/helper-proto.h  and target/arch/helper.h)
   This one expands prototypes for the helper functions.  */

#if defined(CONFIG_FA_QFLEX) || defined(CONFIG_FLEXUS)
/**
 * @brief HELPER(qflex_executed_instruction)
 * location: location of the gen_helper_ in the transalation.
 */
void HELPER(qflex_executed_instruction)(CPUARMState* env, uint64_t pc, int flags, int location) {
    CPUState *cs = CPU(arm_env_get_cpu(env));
    int cur_el = arm_current_el(env);

    switch(location) {
    case QFLEX_EXEC_IN:
        if((unlikely(qflex_loglevel_mask(QFLEX_LOG_KERNEL_EXEC)) && cur_el != 0)
                || (unlikely(qflex_loglevel_mask(QFLEX_LOG_USER_EXEC)) && fa_qflex_is_user_mode())) {
            qemu_log_lock();
            qemu_log("IN  :");
            log_target_disas(cs, pc, 4, flags);
            qemu_log_unlock();
        }
        break;
    case QFLEX_EXEC_OUT:
        if((unlikely(qflex_loglevel_mask(QFLEX_LOG_KERNEL_EXEC)) && cur_el != 0)
                || (unlikely(qflex_loglevel_mask(QFLEX_LOG_USER_EXEC)) && fa_qflex_is_user_mode())) {
            qemu_log_lock();
            qemu_log("OUT :0x%016lx\n", pc);
            qemu_log_unlock();        }
        qflex_update_inst_done(true); break;
    default: break;
    }
}


void HELPER(qflex_magic_ins)(int v) {
    switch(v) {
    case 10: qflex_log_mask_enable(QFLEX_LOG_INT); break;
    case 20: qflex_log_mask_disable(QFLEX_LOG_INT); break;
    case 95: qflex_log_mask_enable(QFLEX_LOG_MAGIC_INSN); break;
    case 109: qflex_log_mask_disable(QFLEX_LOG_MAGIC_INSN); break;
    default: break;
    }
    qflex_log_mask(QFLEX_LOG_MAGIC_INSN,"MAGIC_INST:%u\n", v);
}
#endif /* CONFIG_FA_QFLEX */ /* CONFIG_FLEXUS */

#if defined(CONFIG_FA_QFLEX)
void HELPER(fa_qflex_exception_return)(CPUARMState *env) {
    fa_qflex_update_excp_out(arm_current_el(env));
}
#endif /* CONFIG_FA_QFLEX */

