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
    LDST_L2H,
//} LDST_t;

//typedef enum {
/* Data processing - register */
    DP_REG_LOG_SR,     /* Logical (shifted register) */
    DP_REG_AS_EXT_REG, /* Add/subtract (extended register) */
    DP_REG_AS_REG,     /* Add/subtract (non extended register) */
    DP_REG_AS_CARRY,   /* Add/subtract (with carry) */
    DP_REG_COND_COMP,  /* Conditional compare */
    DP_REG_COMP_SELE,  /* Conditional select */
    DP_REG_DATA_1S,    /* Data-processing (1 source) */
    DP_REG_DATA_2S,    /* Data-processing (2 source) */
    DP_REG_DATA_3S,    /* Data-processing (3 source) */
//} DATA_PROC_REG_t;
/* Data processing - SIMD and floating point */
    // DP_SIMD_FP
    L2H_NR
} L2H;

/* LDST */
typedef enum {
    LDST_EXCLUSIVE, /* Load/store exclusive */
    LDST_REG_LIT,   /* Load register (literal) */
    LDST_SIMD_MULT, /* AdvSIMD load/store multiple structures */
    LDST_SIMD_SING, /* AdvSIMD load/store single structure */
/* Load/store register (all forms) */
    LDST_REG_IMM_UNSCALED, /* Load/store register (unscaled immediate)     */
    LDST_REG_UNPRIVILEGED, /* Load/store register (unprivileged)           */
    LDST_REG_IMM_POST_IDX, /* Load/store register (immediate post-indexed) */
    LDST_REG_IMM_PRE_IDX,  /* Load/store register (immediate pre-indexed)  */
    LDST_REG_ROFFSET,      /* Load/store register (register offset)        */
    LDST_REG_UNSIGNED_IMM, /* Load/store register (unsigned immediate)     */
    LDST_REG_SIMD,         /* Load/store register (SIMD)                   */
/* Load/store pair (all forms) */
    LDST_PAIR_NO_ALLOCATE_OFFSET, /* Load/store no-allocate pair (offset)    */
    LDST_PAIR_POST_IDX,           /* Load/store register pair (post-indexed) */
    LDST_PAIR_PRE_IDX,            /* Load/store register pair (offset)       */
    LDST_PAIR_OFFSET,             /* Load/store register pair (pre-indexed)  */
    LDST_PAIR_SIMD,               /* Load/store register pair (SIMD) */
    LDST_NR
} LDST_t;

typedef struct disas_profile_t {
    L1H l1h;
    L2H l2h; // L2H
    LDST_t ldst;
} disas_profile_t;

typedef struct QFLEXProfileItem {
    int enumerator;
    int enumerator_l1h;
    const char *name;
} QFLEXProfileItem;

typedef struct qflex_profile_t {
    long curr_el_profiles_l1h[L1H_NR];
    long curr_el_profiles_l2h[L2H_NR];
    long global_profiles_l1h[2][L1H_NR];
    long global_profiles_l2h[2][L2H_NR];
    long global_ldst[LDST_NR];
} qflex_profile_t;

extern qflex_profile_t qflex_profile_stats;

const char* qflex_profile_get_string_l1h(L1H l1h);
const char* qflex_profile_get_string_l2h(L2H l2h);
const char* qflex_profile_get_string_ldst(LDST_t ldst);
void qflex_profile_log_l2h_names_csv(void);
void qflex_profile_curr_el_log_stats_verbose(void);
void qflex_profile_curr_el_log_stats_csv(int el);
void qflex_profile_curr_el_reset(void);
void qflex_profile_global_log_stats(void);
void qflex_profile_global_reset(void);
void qflex_profile_disas_a64_insn(uint64_t pc, int flags, uint32_t insn);
