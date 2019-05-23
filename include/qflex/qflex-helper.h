#if defined(CONFIG_FA_QFLEX) || defined(CONFIG_FLEXUS)
#define QFLEX_EXEC_IN   (0)
#define QFLEX_EXEC_OUT  (1)

DEF_HELPER_4(qflex_executed_instruction, void, env, i64, int, int)
DEF_HELPER_1(qflex_magic_ins, void, int)

#endif /* CONFIG_FA_QFLEX */ /* CONFIG_FLEXUS */

#if defined(CONFIG_FA_QFLEX)
DEF_HELPER_1(fa_qflex_exception_return, void, env)
#endif /* CONFIG_FA_QFLEX */
