#include "qflex/qflex-log.h"
int qflex_loglevel = 0;

const QEMULogItem qflex_log_items[] = {
    { QFLEX_LOG_GENERAL, "gen",
      "show general QFLEX actions" },
    { QFLEX_LOG_INTERRUPT, "int",
      "show target assembly code for each compiled TB" },
    { QFLEX_LOG_TB_EXEC, "exec",
      "show instruction for each executed TB" },
    { QFLEX_LOG_MAGIC_INSN, "magic_insn",
      "show when QFLEX magic instrutions are executed" },
    { QFLEX_LOG_FF, "ff",
      "fast-forward cores into user-mode" },
    { QFLEX_LOG_PROFILE, "profile",
      "show when instrutions profile trace when executed" },
    { 0, NULL, NULL },
};

/* takes a comma separated list of log masks. Return 0 if error. */
int qflex_str_to_log_mask(const char *str)
{
    const QEMULogItem *item;
    int mask = 0;
    char **parts = g_strsplit(str, ",", 0);
    char **tmp;

    for (tmp = parts; tmp && *tmp; tmp++) {
        if (g_str_equal(*tmp, "all")) {
            for (item = qflex_log_items; item->mask != 0; item++) {
                mask |= item->mask;
            }
        } else {
            for (item = qflex_log_items; item->mask != 0; item++) {
                if (g_str_equal(*tmp, item->name)) {
                    goto found;
                }
            }
            goto error;
        found:
            mask |= item->mask;
        }
    }

    g_strfreev(parts);
    return mask;

 error:
    g_strfreev(parts);
    return 0;
}


void qflex_print_log_usage(const char *str, FILE *f)
{
    const QEMULogItem *item;
    fprintf(f, "Error QFLEX logging passed as argument: %s\n", str);
    fprintf(f, "Log items options (comma separated):\n");
    for (item = qflex_log_items; item->mask != 0; item++) {
        fprintf(f, "%-15s %s\n", item->name, item->help);
    }
}
