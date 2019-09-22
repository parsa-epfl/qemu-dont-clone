/** HELPER definitions for TCG code generation.
 */
#if defined(TCG_GEN)
DEF_HELPER_4(qflex_executed_instruction, void, env, i64, int, int)
DEF_HELPER_5(qflex_profile, void, env, i64, int, int, int)
DEF_HELPER_1(qflex_magic_insn, void, int)
DEF_HELPER_1(qflex_exception_return, void, env)
#elif !(defined(CONFIG_FLEXUS) || defined(CONFIG_FA_QFLEX)) // Empty definitions when disabled
void HELPER(qflex_executed_instruction)(CPUARMState* env, uint64_t pc, int flags, int location);
void HELPER(qflex_profile)(CPUARMState* env, uint64_t pc, int flags, int l1h, int l2h);
void HELPER(qflex_magic_insn)(int nop_op);
void HELPER(qflex_exception_return)(CPUARMState *env);
#endif

#ifdef CONFIG_FLEXUS
#if defined(TCG_GEN)
// maginc instruction operand
DEF_HELPER_5(flexus_magic_ins, void, int, int, i64, i64, i64)
// No operands
DEF_HELPER_2(flexus_periodic, void, env, int)
// env, pc, target address, ins_size, is user, conditional or not, annulation or not (execute delay slot or not)
DEF_HELPER_7(flexus_insn_fetch, void, env, tl, tl, int, int, int, int)
// env, addr, size, is user, pc, is atomic
DEF_HELPER_6(flexus_ld, void, env, tl, int, int, tl, int)
// env, addr, size, is user, pc, is atomic
DEF_HELPER_6(flexus_st, void, env, tl, int, int, tl, int)
// specific versions for aarch32
// env, pc, target address, ins_size, is user, conditional or not, annulation or not (execute delay slot or not)
DEF_HELPER_7(flexus_insn_fetch_aa32, void, env, tl, i32, int, int, int, int)
// env, addr, size, is user, pc, is atomic
DEF_HELPER_6(flexus_ld_aa32, void, env, i32, int, int, tl, int)
// env, addr, size, is user, pc, is atomic
DEF_HELPER_6(flexus_st_aa32, void, env, i32, int, int, tl, int)
#endif
#endif /* CONFIG_FLEXUS */
