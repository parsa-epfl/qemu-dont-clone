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
   This one defines data structures private to tcg.c.  */

#ifndef HELPER_TCG_H
#define HELPER_TCG_H

#include "exec/helper-head.h"

/* Need one more level of indirection before stringification
   to get all the macros expanded first.  */
#define str(s) #s

#define DEF_HELPER_FLAGS_0(NAME, FLAGS, ret) \
  { .func = HELPER(NAME), .name = str(NAME), .flags = FLAGS, \
    .sizemask = dh_sizemask(ret, 0) },

#define DEF_HELPER_FLAGS_1(NAME, FLAGS, ret, t1) \
  { .func = HELPER(NAME), .name = str(NAME), .flags = FLAGS, \
    .sizemask = dh_sizemask(ret, 0) | dh_sizemask(t1, 1) },

#define DEF_HELPER_FLAGS_2(NAME, FLAGS, ret, t1, t2) \
  { .func = HELPER(NAME), .name = str(NAME), .flags = FLAGS, \
    .sizemask = dh_sizemask(ret, 0) | dh_sizemask(t1, 1) \
    | dh_sizemask(t2, 2) },

#define DEF_HELPER_FLAGS_3(NAME, FLAGS, ret, t1, t2, t3) \
  { .func = HELPER(NAME), .name = str(NAME), .flags = FLAGS, \
    .sizemask = dh_sizemask(ret, 0) | dh_sizemask(t1, 1) \
    | dh_sizemask(t2, 2) | dh_sizemask(t3, 3) },

#define DEF_HELPER_FLAGS_4(NAME, FLAGS, ret, t1, t2, t3, t4) \
  { .func = HELPER(NAME), .name = str(NAME), .flags = FLAGS, \
    .sizemask = dh_sizemask(ret, 0) | dh_sizemask(t1, 1) \
    | dh_sizemask(t2, 2) | dh_sizemask(t3, 3) | dh_sizemask(t4, 4) },

#define DEF_HELPER_FLAGS_5(NAME, FLAGS, ret, t1, t2, t3, t4, t5) \
  { .func = HELPER(NAME), .name = str(NAME), .flags = FLAGS, \
    .sizemask = dh_sizemask(ret, 0) | dh_sizemask(t1, 1) \
    | dh_sizemask(t2, 2) | dh_sizemask(t3, 3) | dh_sizemask(t4, 4) \
    | dh_sizemask(t5, 5) },
#ifdef CONFIG_FLEXUS
#define DEF_HELPER_FLAGS_6(NAME, FLAGS, ret, t1, t2, t3, t4, t5, t6)	\
  { .func = HELPER(NAME), .name = #NAME, .flags = FLAGS, \
    .sizemask = dh_sizemask(ret, 0) | dh_sizemask(t1, 1) \
    | dh_sizemask(t2, 2) | dh_sizemask(t3, 3) | dh_sizemask(t4, 4) \
    | dh_sizemask(t5, 5) | dh_sizemask(t6, 6) },

#define DEF_HELPER_FLAGS_7(NAME, FLAGS, ret, t1, t2, t3, t4, t5, t6, t7)	\
  { .func = HELPER(NAME), .name = #NAME, .flags = FLAGS, \
    .sizemask = dh_sizemask(ret, 0) | dh_sizemask(t1, 1) \
    | dh_sizemask(t2, 2) | dh_sizemask(t3, 3) | dh_sizemask(t4, 4) \
    | dh_sizemask(t5, 5) | dh_sizemask(t6, 6) | dh_sizemask(t7, 7) },
#endif

#include "helper.h"
#include "trace/generated-helpers.h"
#include "tcg-runtime.h"

#undef str
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
#endif /* HELPER_TCG_H */
