#ifndef QFLEX_LOG_H
#define QFLEX_LOG_H

#include "qemu/osdep.h"
#include "qemu/log.h"

extern int qflex_loglevel;
extern int qflex_iloop;
extern int qflex_iExit;

#define QFLEX_LOG_GENERAL       (1 << 0)
#define QFLEX_LOG_INTERRUPT     (1 << 1)
#define QFLEX_LOG_TB_EXEC       (1 << 2)
#define QFLEX_LOG_MAGIC_INSN    (1 << 3)
#define QFLEX_LOG_FF            (1 << 4)
#define QFLEX_LOG_PROFILE_INST  (1 << 5)
#define QFLEX_LOG_PROFILE_EL    (1 << 6)
#define QFLEX_LOG_FILE_ACCESS   (1 << 7)
#ifdef CONFIG_FA_QFLEX
#define FA_QFLEX_LOG_CMDS      (1 << (0 + 16))
#define FA_QFLEX_LOG_SIM       (1 << (1 + 16))
#endif

/* Returns true if a bit is set in the current loglevel mask
 */
static inline bool qflex_loglevel_mask(int mask)
{
    return (qflex_loglevel & mask) != 0;
}

/* Logging functions: */
int qflex_str_to_log_mask(const char *str);
void qflex_print_log_usage(const char *str, FILE *f);

/* log only if a bit is set on the current loglevel mask:
 * @mask: bit to check in the mask
 * @fmt: printf-style format string * @args: optional arguments for format string
 */
#define qflex_log_mask(MASK, FMT, ...)                  \
    do {                                                \
        if (unlikely(qflex_loglevel_mask(MASK))) {      \
            qemu_log(FMT, ## __VA_ARGS__);              \
        }                                               \
    } while (0)

#define qflex_log_mask_enable(MASK)     \
    do { qflex_loglevel |= MASK; } while (0)

#define qflex_log_mask_disable(MASK)    \
    do { qflex_loglevel &= ~MASK; } while (0)

static inline void qflex_set_log(int mask) { qflex_loglevel = mask; }

#endif /* QFLEX_LOG_H */

