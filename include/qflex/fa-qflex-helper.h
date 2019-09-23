#ifndef FA_QFLEX_ARM_H
#define FA_QFLEX_ARM_H

/* Functions that are only present as static functions in target/arch files
 * Make them available to get called by QFLEX
 */

#define QFLEX_GET_F(func) glue(qflex_get_, func)

/* op_helper.c
 */
void QFLEX_GET_F(deliver_fault)(ARMCPU *cpu, vaddr addr, MMUAccessType access_type,
                                uint32_t fsr, uint32_t fsc, ARMMMUFaultInfo *fi);

/* helper.c
 */
bool QFLEX_GET_F(get_phys_addr)(CPUARMState *env, target_ulong address,
                                MMUAccessType access_type, ARMMMUIdx mmu_idx,
                                hwaddr *phys_ptr, MemTxAttrs *attrs, int *prot,
                                target_ulong *page_size, uint32_t *fsr,
                                ARMMMUFaultInfo *fi);

#endif /* FA_QFLEX_ARM_H */
