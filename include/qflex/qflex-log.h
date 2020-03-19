//  DO-NOT-REMOVE begin-copyright-block
// QFlex consists of several software components that are governed by various
// licensing terms, in addition to software that was developed internally.
// Anyone interested in using QFlex needs to fully understand and abide by the
// licenses governing all the software components.
// 
// ### Software developed externally (not by the QFlex group)
// 
//     * [NS-3] (https://www.gnu.org/copyleft/gpl.html)
//     * [QEMU] (http://wiki.qemu.org/License)
//     * [SimFlex] (http://parsa.epfl.ch/simflex/)
//     * [GNU PTH] (https://www.gnu.org/software/pth/)
// 
// ### Software developed internally (by the QFlex group)
// **QFlex License**
// 
// QFlex
// Copyright (c) 2020, Parallel Systems Architecture Lab, EPFL
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
// 
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright notice,
//       this list of conditions and the following disclaimer in the documentation
//       and/or other materials provided with the distribution.
//     * Neither the name of the Parallel Systems Architecture Laboratory, EPFL,
//       nor the names of its contributors may be used to endorse or promote
//       products derived from this software without specific prior written
//       permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE PARALLEL SYSTEMS ARCHITECTURE LABORATORY,
// EPFL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//  DO-NOT-REMOVE end-copyright-block
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

#define QFLEX_INIT_LOOP() do {  \
    qflex_iExit = 0;                  \
} while(0)

#define QFLEX_CHECK_LOOP(cpu) do {   \
    if (qflex_iloop > 0) {           \
        if (++qflex_iExit > qflex_iloop) { \
            qflex_iExit = 0;               \
            qemu_cpu_kick(cpu);      \
        }                            \
    }                                \
} while(0)

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

