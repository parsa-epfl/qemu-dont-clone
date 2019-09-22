#include "qemu/osdep.h"

#include "cpu.h"
#include "exec/exec-all.h"
#include "tcg-op.h"
#include "qemu/log.h"
#include "arm_ldst.h"
#include "translate.h"
#include "internals.h"
#include "qemu/host-utils.h"

#include "qflex/qflex-profiling.h"
#include "qflex/qflex-helper.h"


int qflex_insn_profiles_l1h[2][L1H_NR] = {};
int qflex_insn_profiles_l2h[2][L1H_NR][L2H_NR] = {};

const char* qflex_profile_get_char_l1h(L1H l1h) {
    const char* name = "UNDEF";

    switch(l1h) {
    case      DATA_PROC_IM: name = "DATA_PROC_IM";      break; /* Data processing - immediate */
    case         B_EXC_SYS: name = "B_EXC_SYS";         break; /* Branch, exception generation and system insns */
    case              LDST: name = "LDST";              break; /* Loads and stores */
    case     DATA_PROC_REG: name = "DATA_PROC_REG";     break; /* Data processing - register */
    case DATA_PROC_SIMD_FP: name = "DATA_PROC_SIMD_FP"; break; /* Data processing - SIMD and floating point */
    default: break;
    }
    return name;
}

const char* qflex_profile_get_char_l2h(L1H l1h, L2H l2h) {
    const char* name = "UNDEF";
    switch(l1h) {
    case DATA_PROC_IM:       /* Data processing - immediate */
        switch(l2h) {
        case  DP_IMM_PC: name = "DP_IMM_PC";  break; /* PC-rel. addressing */
        case  DP_IMM_AS: name = "DP_IMM_AS";  break; /* Add/subtract (immediate) */
        case DP_IMM_LOG: name = "DP_IMM_LOG"; break; /* Logical (immediate) */
        case DP_IMM_MOV: name = "DP_IMM_MOV"; break; /* Move wide (immediate) */
        case DP_IMM_BIT: name = "DP_IMM_BIT"; break; /* Bitfield */
        case DP_IMM_EXT: name = "DP_IMM_EXT"; break; /* Extract */
        default: break;
        }

    case B_EXC_SYS:          /* Branch, exception generation and system insns */
        switch(l2h) {
        case BRES_UNCONDITIONAL_BR_IMM: name = "BRES_UNCOND_BR_IMM"; break; /* Unconditional branch (immediate) */
        case BRES_COMPARE_N_BR_IMM:     name = "BRES_COMP_N_BR_IMM"; break; /* Compare & branch (immediate) */
        case BRES_TEST_N_BR_IMM:        name = "BRES_TEST_N_BR_IMM"; break; /* Test & branch (immediate) */
        case BRES_CONDITIONAL_BR_IMM:   name = "BRES_COND_BR_IMM";   break; /* Conditional branch (immediate) */
        case BRES_SYSTEM:               name = "BRES_SYSTEM";        break; /* System */
        case BRES_EXECP:                name = "BRES_EXECP";         break; /* Exception generation */
        case BRES_UNCONDITIONAL_BR_REG: name = "BRES_UNCOND_BR_REG"; break; /* Unconditional branch (register) */
        default: break;
        }

    case LDST:               /* Loads and stores */
        switch(l2h) {
        case LDST_EXCLUSIVE: name = "LDST_EXCLUSIVE";  break; /* Load/store exclusive */
        case LDST_REG_LIT:   name = "LDST_REG_LIT";    break; /* Load register (literal) */
        case LDST_PAIR:      name = "LDST_PAIR";       break; /* Load/store pair (all forms) */
        case LDST_REG:       name = "LDST_REG";        break; /* Load/store register (all forms) */
        case LDST_SIMD_MULT: name = "LDST_SIMD_MULT "; break; /* AdvSIMD load/store multiple structures */
        case LDST_SIMD_SING: name = "LDST_SIMD_SING "; break; /* AdvSIMD load/store single structure */
        default: break;
        }

    case DATA_PROC_REG:      /* Data processing - register */
        switch(l2h) {
        case DP_REG_LOG_SR:     name = "DP_REG_LOG_SR";     break; /* Logical (shifted register) */
        case DP_REG_AS_EXT_REG: name = "DP_REG_AS_EXT_REG"; break; /* Add/subtract (extended register) */
        case DP_REG_AS_REG:     name = "DP_REG_AS_REG";     break; /* Add/subtract (non extended register) */
        case DP_REG_DATA_3S:    name = "DP_REG_DATA_3S";    break; /* Data-processing (3 source) */
        case DP_REG_AS_CARRY:   name = "DP_REG_AS_CARRY";   break; /* Add/subtract (with carry) */
        case DP_REG_COND_COMP:  name = "DP_REG_COND_COMP";  break; /* Conditional compare */
        case DP_REG_COMP_SELE:  name = "DP_REG_COMP_SELE";  break; /* Conditional select */
        case DP_REG_DATA_1S:    name = "DP_REG_DATA_1S";    break; /* Data-processing (1 source) */
        case DP_REG_DATA_2S:    name = "DP_REG_DATA_2S";    break; /* Data-processing (2 source) */
        default: break;
        }

    case DATA_PROC_SIMD_FP:  /* Data processing - SIMD and floating point */
        switch(l2h) {
        default: break;
        }

    default: break;
    }

    return name;
}

#if defined(CONFIG_FLEXUS) || defined(CONFIG_FA_QFLEX)
static disas_profile_t insn_profile;

/* L2H: Data processing - register */
static void disas_data_proc_reg(DisasContext *s, uint32_t insn)
{
    switch (extract32(insn, 24, 5)) {
    case 0x0a: /* Logical (shifted register) */
        insn_profile.l2h = DP_REG_LOG_SR;
        // disas_logic_reg(s, insn);
        break;
    case 0x0b: /* Add/subtract */
        if (insn & (1 << 21)) { /** (extended register) */
            insn_profile.l2h = DP_REG_AS_EXT_REG;
            // disas_add_sub_ext_reg(s, insn);
        } else {                /** (non extended register) */
            insn_profile.l2h = DP_REG_AS_REG;
            // disas_add_sub_reg(s, insn);
        }
        break;
    case 0x1b: /* Data-processing (3 source) */
        insn_profile.l2h = DP_REG_DATA_3S;
        // disas_data_proc_3src(s, insn);
        break;
    case 0x1a:
        switch (extract32(insn, 21, 3)) {
        case 0x0: /* Add/subtract (with carry) */
            insn_profile.l2h = DP_REG_AS_CARRY;
            // disas_adc_sbc(s, insn);
            break;
        case 0x2: /* Conditional compare */
            insn_profile.l2h = DP_REG_COND_COMP;
            // disas_cc(s, insn); /* both imm and reg forms */
            break;
        case 0x4: /* Conditional select */
            insn_profile.l2h = DP_REG_COMP_SELE;
            // disas_cond_select(s, insn);
            break;
        case 0x6: /* Data-processing */
            if (insn & (1 << 30)) { /** (1 source) */
                insn_profile.l2h = DP_REG_DATA_1S;
                // disas_data_proc_1src(s, insn);
            } else {                /** (2 source) */
                insn_profile.l2h = DP_REG_DATA_2S;
                // disas_data_proc_2src(s, insn);
            }
            break;
        default:
            // unallocated_encoding(s);
            break;
        }
        break;
    default:
        // unallocated_encoding(s);
        break;
    }
}

/* L2; Loads and stores */
static void disas_ldst(DisasContext *s, uint32_t insn)
{
    switch (extract32(insn, 24, 6)) {
    case 0x08: /* Load/store exclusive */
        insn_profile.l2h = LDST_EXCLUSIVE;
        // disas_ldst_excl(s, insn);
        break;
    case 0x18: case 0x1c: /* Load register (literal) */
        insn_profile.l2h = LDST_REG_LIT;
        // disas_ld_lit(s, insn);
        break;
    case 0x28: case 0x29:
    case 0x2c: case 0x2d: /* Load/store pair (all forms) */
        insn_profile.l2h = LDST_PAIR;
        // disas_ldst_pair(s, insn);
        break;
    case 0x38: case 0x39:
    case 0x3c: case 0x3d: /* Load/store register (all forms) */
        insn_profile.l2h = LDST_REG;
        // disas_ldst_reg(s, insn);
        break;
    case 0x0c: /* AdvSIMD load/store multiple structures */
        insn_profile.l2h = LDST_SIMD_MULT;
        // disas_ldst_multiple_struct(s, insn);
        break;
    case 0x0d: /* AdvSIMD load/store single structure */
        insn_profile.l2h = LDST_SIMD_SING;
        // disas_ldst_single_struct(s, insn);
        break;
    default:
        // unallocated_encoding(s);
        break;
    }
}

/* Branch, exception generation and system insns */
/* L2; Branches, exception generating and system instructions */
static void disas_b_exc_sys(DisasContext *s, uint32_t insn)
{
    switch (extract32(insn, 25, 7)) {
    case 0x0a: case 0x0b:
    case 0x4a: case 0x4b: /* Unconditional branch (immediate) */
        insn_profile.l2h = BRES_UNCONDITIONAL_BR_IMM;
        // disas_uncond_b_imm(s, insn);
        break;
    case 0x1a: case 0x5a: /* Compare & branch (immediate) */
        insn_profile.l2h = BRES_COMPARE_N_BR_IMM;
        // disas_comp_b_imm(s, insn);
        break;
    case 0x1b: case 0x5b: /* Test & branch (immediate) */
        insn_profile.l2h = BRES_TEST_N_BR_IMM;
        // disas_test_b_imm(s, insn);
        break;
    case 0x2a: /* Conditional branch (immediate) */
        insn_profile.l2h = BRES_CONDITIONAL_BR_IMM;
        // disas_cond_b_imm(s, insn);
        break;
    case 0x6a: /* Exception generation */
        if (insn & (1 << 24)) {
            insn_profile.l2h = BRES_SYSTEM;
            // disas_system(s, insn);
        } else {
            insn_profile.l2h = BRES_EXECP;
            // disas_exc(s, insn);
        }
        break;
    case 0x6b: /* Unconditional branch (register) */
        insn_profile.l2h = BRES_UNCONDITIONAL_BR_REG;
        // disas_uncond_b_reg(s, insn);
        break;
    default:
        // unallocated_encoding(s);
        break;
    }
}


/* L2; Data processing - immediate */
static void disas_data_proc_imm(DisasContext *s, uint32_t insn)
{
    switch (extract32(insn, 23, 6)) {
    case 0x20: case 0x21: /* PC-rel. addressing */
        insn_profile.l2h = DP_IMM_PC;
        // disas_pc_rel_adr(s, insn);
        break;
    case 0x22: case 0x23: /* Add/subtract (immediate) */
        insn_profile.l2h = DP_IMM_AS;
        // disas_add_sub_imm(s, insn);
        break;
    case 0x24: /* Logical (immediate) */
        insn_profile.l2h = DP_IMM_LOG;
        // disas_logic_imm(s, insn);
        break;
    case 0x25: /* Move wide (immediate) */
        insn_profile.l2h = DP_IMM_MOV;
        // disas_movw_imm(s, insn);
        break;
    case 0x26: /* Bitfield */
        insn_profile.l2h = DP_IMM_BIT;
        // disas_bitfield(s, insn);
        break;
    case 0x27: /* Extract */
        insn_profile.l2h = DP_IMM_EXT;
        // disas_extract(s, insn);
        break;
    default:
        // unallocated_encoding(s);
        break;
    }
}

/* L1; C3.1 A64 instruction index by encoding */
void qflex_profile_disas_a64_insn(CPUARMState *env,  uint64_t pc, int flags, uint32_t insn)
{
    DisasContext *s = NULL; // Pass empty argument to maintain similar flow to translate-a64.c

    switch (extract32(insn, 25, 4)) {
    case 0x0: case 0x1: case 0x2: case 0x3: /* UNALLOCATED */
        // unallocated_encoding(s);
        break;
    case 0x8: case 0x9: /* Data processing - immediate */
        insn_profile.l1h = DATA_PROC_IM;
        disas_data_proc_imm(s, insn);
        break;
    case 0xa: case 0xb: /* Branch, exception generation and system insns */
        insn_profile.l1h = B_EXC_SYS;
        disas_b_exc_sys(s, insn);
        break;
    case 0x4: case 0x6: case 0xc: case 0xe:      /* Loads and stores */
        insn_profile.l1h = LDST;
        disas_ldst(s, insn);
        break;
    case 0x5: case 0xd:      /* Data processing - register */
        insn_profile.l1h = DATA_PROC_REG;
        disas_data_proc_reg(s, insn);
        break;
    case 0x7: case 0xf:      /* Data processing - SIMD and floating point */
        insn_profile.l1h = DATA_PROC_SIMD_FP;
        // disas_data_proc_simd_fp(s, insn);
        break;
    default:
        //assert(FALSE); /* all 15 cases should be handled above */
        break;
    }
    GEN_HELPER(qflex_profile)(cpu_env, tcg_const_i64(pc), tcg_const_i32(flags),
                                tcg_const_i32(insn_profile.l1h), tcg_const_i32(insn_profile.l2h));
}
#else // Empty definitions when qflex disabled
void qflex_profile_disas_a64_insn(CPUARMState *env,  uint64_t pc, int flags, uint32_t insn){return;}
#endif

/* Get's rid of 'defined but not used' warning */
void wno_unused_translate_h(void);
void wno_unused_translate_h(void)
{
  /* don't need to actually call the functions to avoid the warnings */
  (void)&disas_set_insn_syndrome;
  return;
}
