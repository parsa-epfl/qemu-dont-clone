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
   Used by other helper files.

   Targets should use DEF_HELPER_N and DEF_HELPER_FLAGS_N to declare helper
   functions.  Names should be specified without the helper_ prefix, and
   the return and argument types specified.  3 basic types are understood
   (i32, i64 and ptr).  Additional aliases are provided for convenience and
   to match the types used by the C helper implementation.

   The target helper.h should be included in all files that use/define
   helper functions.  THis will ensure that function prototypes are
   consistent.  In addition it should be included an extra two times for
   helper.c, defining:
    GEN_HELPER 1 to produce op generation functions (gen_helper_*)
    GEN_HELPER 2 to do runtime registration helper functions.
 */

#ifndef EXEC_HELPER_HEAD_H
#define EXEC_HELPER_HEAD_H

#define HELPER(name) glue(helper_, name)

#define GET_TCGV_i32 GET_TCGV_I32
#define GET_TCGV_i64 GET_TCGV_I64
#define GET_TCGV_ptr GET_TCGV_PTR

/* Some types that make sense in C, but not for TCG.  */
#define dh_alias_i32 i32
#define dh_alias_s32 i32
#define dh_alias_int i32
#define dh_alias_i64 i64
#define dh_alias_s64 i64
#define dh_alias_f32 i32
#define dh_alias_f64 i64
#define dh_alias_ptr ptr
#define dh_alias_void void
#define dh_alias_noreturn noreturn
#define dh_alias(t) glue(dh_alias_, t)

#define dh_ctype_i32 uint32_t
#define dh_ctype_s32 int32_t
#define dh_ctype_int int
#define dh_ctype_i64 uint64_t
#define dh_ctype_s64 int64_t
#define dh_ctype_f32 float32
#define dh_ctype_f64 float64
#define dh_ctype_ptr void *
#define dh_ctype_void void
#define dh_ctype_noreturn void QEMU_NORETURN
#define dh_ctype(t) dh_ctype_##t

#ifdef NEED_CPU_H
# ifdef TARGET_LONG_BITS
#  if TARGET_LONG_BITS == 32
#   define dh_alias_tl i32
#  else
#   define dh_alias_tl i64
#  endif
# endif
# define dh_alias_env ptr
# define dh_ctype_tl target_ulong
# define dh_ctype_env CPUArchState *
#endif

/* We can't use glue() here because it falls foul of C preprocessor
   recursive expansion rules.  */
#define dh_retvar_decl0_void void
#define dh_retvar_decl0_noreturn void
#define dh_retvar_decl0_i32 TCGv_i32 retval
#define dh_retvar_decl0_i64 TCGv_i64 retval
#define dh_retvar_decl0_ptr TCGv_ptr retval
#define dh_retvar_decl0(t) glue(dh_retvar_decl0_, dh_alias(t))

#define dh_retvar_decl_void
#define dh_retvar_decl_noreturn
#define dh_retvar_decl_i32 TCGv_i32 retval,
#define dh_retvar_decl_i64 TCGv_i64 retval,
#define dh_retvar_decl_ptr TCGv_ptr retval,
#define dh_retvar_decl(t) glue(dh_retvar_decl_, dh_alias(t))

#define dh_retvar_void TCG_CALL_DUMMY_ARG
#define dh_retvar_noreturn TCG_CALL_DUMMY_ARG
#define dh_retvar_i32 GET_TCGV_i32(retval)
#define dh_retvar_i64 GET_TCGV_i64(retval)
#define dh_retvar_ptr GET_TCGV_ptr(retval)
#define dh_retvar(t) glue(dh_retvar_, dh_alias(t))

#define dh_is_64bit_void 0
#define dh_is_64bit_noreturn 0
#define dh_is_64bit_i32 0
#define dh_is_64bit_i64 1
#define dh_is_64bit_ptr (sizeof(void *) == 8)
#define dh_is_64bit(t) glue(dh_is_64bit_, dh_alias(t))

#define dh_is_signed_void 0
#define dh_is_signed_noreturn 0
#define dh_is_signed_i32 0
#define dh_is_signed_s32 1
#define dh_is_signed_i64 0
#define dh_is_signed_s64 1
#define dh_is_signed_f32 0
#define dh_is_signed_f64 0
#define dh_is_signed_tl  0
#define dh_is_signed_int 1
/* ??? This is highly specific to the host cpu.  There are even special
   extension instructions that may be required, e.g. ia64's addp4.  But
   for now we don't support any 64-bit targets with 32-bit pointers.  */
#define dh_is_signed_ptr 0
#define dh_is_signed_env dh_is_signed_ptr
#define dh_is_signed(t) dh_is_signed_##t

#define dh_sizemask(t, n) \
  ((dh_is_64bit(t) << (n*2)) | (dh_is_signed(t) << (n*2+1)))

#define dh_arg(t, n) \
  glue(GET_TCGV_, dh_alias(t))(glue(arg, n))

#define dh_arg_decl(t, n) glue(TCGv_, dh_alias(t)) glue(arg, n)

#define DEF_HELPER_0(name, ret) \
    DEF_HELPER_FLAGS_0(name, 0, ret)
#define DEF_HELPER_1(name, ret, t1) \
    DEF_HELPER_FLAGS_1(name, 0, ret, t1)
#define DEF_HELPER_2(name, ret, t1, t2) \
    DEF_HELPER_FLAGS_2(name, 0, ret, t1, t2)
#define DEF_HELPER_3(name, ret, t1, t2, t3) \
    DEF_HELPER_FLAGS_3(name, 0, ret, t1, t2, t3)
#define DEF_HELPER_4(name, ret, t1, t2, t3, t4) \
    DEF_HELPER_FLAGS_4(name, 0, ret, t1, t2, t3, t4)
#define DEF_HELPER_5(name, ret, t1, t2, t3, t4, t5) \
    DEF_HELPER_FLAGS_5(name, 0, ret, t1, t2, t3, t4, t5)
#ifdef CONFIG_FLEXUS
#define DEF_HELPER_6(name, ret, t1, t2, t3, t4, t5, t6) \
  DEF_HELPER_FLAGS_6(name, 0, ret, t1, t2, t3, t4, t5, t6)
#define DEF_HELPER_7(name, ret, t1, t2, t3, t4, t5, t6, t7)	\
  DEF_HELPER_FLAGS_7(name, 0, ret, t1, t2, t3, t4, t5, t6, t7)
#endif
/* MAX_OPC_PARAM_IARGS must be set to n if last entry is DEF_HELPER_FLAGS_n. */

#endif /* EXEC_HELPER_HEAD_H */
