#if defined(CONFIG_FA_QFLEX) || defined(CONFIG_FLEXUS)
DEF_HELPER_1(qflex_magic_ins, void, int)
DEF_HELPER_3(qflex_executed_instruction, void, env, i64, int)
#endif /* CONFIG_FA_QFLEX */ /* CONFIG_FLEXUS */

#if defined(CONFIG_FA_QFLEX)
DEF_HELPER_1(fa_qflex_exception_return, void, env)
#endif /* CONFIG_FA_QFLEX */
