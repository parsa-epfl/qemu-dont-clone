#include "qflex/qflex.h"

/* L1; C3.1 A64 instruction index by encoding */
typedef enum {
    DATA_PROC_IM,  /* Data processing - immediate */
    B_EXC_SYS,     /* Branch, exception generation and system insns */
    LDST,          /* Loads and stores */
    DATA_PROC_REG, /* Data processing - register */
    DATA_PROC_SIMD_FP, /* Data processing - SIMD and floating point */
    L1H_NR
} L1H;

/* L2H */
typedef enum {
/* Data processing - immediate */
    DP_IMM_PC,  /* PC-rel. addressing */
    DP_IMM_AS,  /* Add/subtract (immediate) */
    DP_IMM_LOG, /* Logical (immediate) */
    DP_IMM_MOV, /* Move wide (immediate) */
    DP_IMM_BIT, /* Bitfield */
    DP_IMM_EXT,  /* Extract */
//} DATA_PROC_IM_t;

//typedef enum {
/* Branch, exception generation and system insns */
    BRES_UNCONDITIONAL_BR_IMM, /* Unconditional branch (immediate) */
    BRES_COMPARE_N_BR_IMM,     /* Compare & branch (immediate) */
    BRES_TEST_N_BR_IMM,        /* Test & branch (immediate) */
    BRES_CONDITIONAL_BR_IMM,   /* Conditional branch (immediate) */
    BRES_SYSTEM,               /* System */
    BRES_EXECP,                /* Exception generation */
    BRES_UNCONDITIONAL_BR_REG, /* Unconditional branch (register) */
//} B_EXC_SYS_t;

//typedef enum {
/* Loads and stores */
    LDST_EXCLUSIVE, /* Load/store exclusive */
    LDST_REG_LIT,   /* Load register (literal) */
    LDST_PAIR,      /* Load/store pair (all forms) */
    LDST_REG,       /* Load/store register (all forms) */
    LDST_SIMD_MULT, /* AdvSIMD load/store multiple structures */
    LDST_SIMD_SING, /* AdvSIMD load/store single structure */
//} LDST_t;

//typedef enum {
/* Data processing - register */
    DP_REG_LOG_SR,    /* Logical (shifted register) */
    /* Add/subtract */
    DP_REG_AS_EXT_REG,  /** (extended register) */
    DP_REG_AS_REG,      /** (non extended register) */
    DP_REG_DATA_3S,   /* Data-processing (3 source) */
    DP_REG_AS_CARRY,  /* Add/subtract (with carry) */
    DP_REG_COND_COMP, /* Conditional compare */
    DP_REG_COMP_SELE, /* Conditional select */
    /* Data-processing */
    DP_REG_DATA_1S,     /** (1 source) */
    DP_REG_DATA_2S,     /** (2 source) */
//} DATA_PROC_REG_t;
/* Data processing - SIMD and floating point */
    // DP_SIMD_FP
    L2H_NR
} L2H;

typedef struct disas_profile_t {
    L1H l1h;
    L2H l2h; // L2H
} disas_profile_t;


extern int qflex_insn_profiles_l1h[2][L1H_NR];
extern int qflex_insn_profiles_l2h[2][L1H_NR][L2H_NR];

const char* qflex_profile_get_char_l1h(L1H l1h);
const char* qflex_profile_get_char_l2h(L1H l1h, L2H l2h);
void qflex_profile_disas_a64_insn(CPUARMState *env, uint64_t pc, int flags, uint32_t insn);
