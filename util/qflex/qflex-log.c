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
#include "qflex/qflex-log.h"
int qflex_loglevel = 0;
int qflex_iloop = 0;
int qflex_iExit = 0;

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
        } else if (g_str_has_prefix(*tmp, "loop=") && (*tmp)[5] != '\0') {
            char* subs = strndup(&(*tmp)[5],  (strlen((*tmp))-5));
            qflex_iloop = atoi(subs);
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
