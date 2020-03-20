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
/* Helper file for declaring TCG helper functions.
   This one expands generation functions for tcg opcodes.  */

#ifndef HELPER_GEN_H
#define HELPER_GEN_H

#include "exec/helper-head.h"

#define DEF_HELPER_FLAGS_0(name, flags, ret)                            \
static inline void glue(gen_helper_, name)(dh_retvar_decl0(ret))        \
{                                                                       \
  tcg_gen_callN(&tcg_ctx, HELPER(name), dh_retvar(ret), 0, NULL);       \
}

#define DEF_HELPER_FLAGS_1(name, flags, ret, t1)                        \
static inline void glue(gen_helper_, name)(dh_retvar_decl(ret)          \
    dh_arg_decl(t1, 1))                                                 \
{                                                                       \
  TCGArg args[1] = { dh_arg(t1, 1) };                                   \
  tcg_gen_callN(&tcg_ctx, HELPER(name), dh_retvar(ret), 1, args);       \
}

#define DEF_HELPER_FLAGS_2(name, flags, ret, t1, t2)                    \
static inline void glue(gen_helper_, name)(dh_retvar_decl(ret)          \
    dh_arg_decl(t1, 1), dh_arg_decl(t2, 2))                             \
{                                                                       \
  TCGArg args[2] = { dh_arg(t1, 1), dh_arg(t2, 2) };                    \
  tcg_gen_callN(&tcg_ctx, HELPER(name), dh_retvar(ret), 2, args);       \
}

#define DEF_HELPER_FLAGS_3(name, flags, ret, t1, t2, t3)                \
static inline void glue(gen_helper_, name)(dh_retvar_decl(ret)          \
    dh_arg_decl(t1, 1), dh_arg_decl(t2, 2), dh_arg_decl(t3, 3))         \
{                                                                       \
  TCGArg args[3] = { dh_arg(t1, 1), dh_arg(t2, 2), dh_arg(t3, 3) };     \
  tcg_gen_callN(&tcg_ctx, HELPER(name), dh_retvar(ret), 3, args);       \
}

#define DEF_HELPER_FLAGS_4(name, flags, ret, t1, t2, t3, t4)            \
static inline void glue(gen_helper_, name)(dh_retvar_decl(ret)          \
    dh_arg_decl(t1, 1), dh_arg_decl(t2, 2),                             \
    dh_arg_decl(t3, 3), dh_arg_decl(t4, 4))                             \
{                                                                       \
  TCGArg args[4] = { dh_arg(t1, 1), dh_arg(t2, 2),                      \
                     dh_arg(t3, 3), dh_arg(t4, 4) };                    \
  tcg_gen_callN(&tcg_ctx, HELPER(name), dh_retvar(ret), 4, args);       \
}

#define DEF_HELPER_FLAGS_5(name, flags, ret, t1, t2, t3, t4, t5)        \
static inline void glue(gen_helper_, name)(dh_retvar_decl(ret)          \
    dh_arg_decl(t1, 1),  dh_arg_decl(t2, 2), dh_arg_decl(t3, 3),        \
    dh_arg_decl(t4, 4), dh_arg_decl(t5, 5))                             \
{                                                                       \
  TCGArg args[5] = { dh_arg(t1, 1), dh_arg(t2, 2), dh_arg(t3, 3),       \
                     dh_arg(t4, 4), dh_arg(t5, 5) };                    \
  tcg_gen_callN(&tcg_ctx, HELPER(name), dh_retvar(ret), 5, args);       \
}


#ifdef CONFIG_FLEXUS
#define DEF_HELPER_FLAGS_6(name, flags, ret, t1, t2, t3, t4, t5, t6)	\
static inline void glue(gen_helper_, name)(dh_retvar_decl(ret)          \
    dh_arg_decl(t1, 1),  dh_arg_decl(t2, 2), dh_arg_decl(t3, 3),        \
    dh_arg_decl(t4, 4), dh_arg_decl(t5, 5), dh_arg_decl(t6, 6))         \
{                                                                       \
  TCGArg args[6] = { dh_arg(t1, 1), dh_arg(t2, 2), dh_arg(t3, 3),       \
                     dh_arg(t4, 4), dh_arg(t5, 5), dh_arg(t6, 6) };	\
  tcg_gen_callN(&tcg_ctx, HELPER(name), dh_retvar(ret), 6, args);       \
}

#define DEF_HELPER_FLAGS_7(name, flags, ret, t1, t2, t3, t4, t5, t6, t7)	\
static inline void glue(gen_helper_, name)(dh_retvar_decl(ret)          \
    dh_arg_decl(t1, 1),  dh_arg_decl(t2, 2), dh_arg_decl(t3, 3),        \
    dh_arg_decl(t4, 4), dh_arg_decl(t5, 5), dh_arg_decl(t6, 6), dh_arg_decl(t7, 7)) \
{                                                                       \
  TCGArg args[7] = { dh_arg(t1, 1), dh_arg(t2, 2), dh_arg(t3, 3),       \
		     dh_arg(t4, 4), dh_arg(t5, 5), dh_arg(t6, 6),       \
                     dh_arg(t7, 7)};								\
  tcg_gen_callN(&tcg_ctx, HELPER(name), dh_retvar(ret), 7, args);       \
}
#endif

#include "helper.h"
#include "trace/generated-helpers.h"
#include "trace/generated-helpers-wrappers.h"
#include "tcg-runtime.h"

#undef DEF_HELPER_FLAGS_0
#undef DEF_HELPER_FLAGS_1
#undef DEF_HELPER_FLAGS_2
#undef DEF_HELPER_FLAGS_3
#undef DEF_HELPER_FLAGS_4
#undef DEF_HELPER_FLAGS_5

#ifdef CONFIG_FLEXUS
#undef DEF_HELPER_FLAGS_6
#undef DEF_HELPER_FLAGS_7
#endif

#undef GEN_HELPER

#endif /* HELPER_GEN_H */
