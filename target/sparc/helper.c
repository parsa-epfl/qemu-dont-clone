/*
 *  Misc Sparc helpers
 *
 *  Copyright (c) 2003-2005 Fabrice Bellard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "qemu/osdep.h"
#include "cpu.h"
#include "exec/exec-all.h"
#include "qemu/host-utils.h"
#include "exec/helper-proto.h"
#include "sysemu/sysemu.h"

#if defined(CONFIG_FLEXUS) || defined(CONFIG_TEST_DETERMINISM) || defined(CONFIG_TEST_TIME)
#define QEMUFLEX_PROTOTYPES
#define QEMUFLEX_QEMU_INTERNAL
#include "../libqemuflex/api.h"
#include "exec/memory.h"
#include "exec/address-spaces.h"
#include "exec/cpu_ldst.h"

#include "exec/cputlb.h"
#endif
#ifdef CONFIG_SIAVASH
//SIA
void cpu_write_register( void *env_ptr, int reg_index, unsigned *reg_size, uint64_t value ) {
	assert(0);
}
//End SIA
#endif
void cpu_raise_exception_ra(CPUSPARCState *env, int tt, uintptr_t ra)
{
    CPUState *cs = CPU(sparc_env_get_cpu(env));

    cs->exception_index = tt;
    cpu_loop_exit_restore(cs, ra);
}

void helper_raise_exception(CPUSPARCState *env, int tt)
{
    CPUState *cs = CPU(sparc_env_get_cpu(env));

    cs->exception_index = tt;
    cpu_loop_exit(cs);
}

void helper_debug(CPUSPARCState *env)
{
    CPUState *cs = CPU(sparc_env_get_cpu(env));

    cs->exception_index = EXCP_DEBUG;
    cpu_loop_exit(cs);
}

#ifdef TARGET_SPARC64
void helper_tick_set_count(void *opaque, uint64_t count)
{
#if !defined(CONFIG_USER_ONLY)
    cpu_tick_set_count(opaque, count);
#endif
}

uint64_t helper_tick_get_count(CPUSPARCState *env, void *opaque, int mem_idx)
{
#if !defined(CONFIG_USER_ONLY)
    CPUTimer *timer = opaque;

    if (timer->npt && mem_idx < MMU_KERNEL_IDX) {
        cpu_raise_exception_ra(env, TT_PRIV_INSN, GETPC());
    }

    return cpu_tick_get_count(timer);
#else
    return 0;
#endif
}

void helper_tick_set_limit(void *opaque, uint64_t limit)
{
#if !defined(CONFIG_USER_ONLY)
    cpu_tick_set_limit(opaque, limit);
#endif
}
#endif

static target_ulong do_udiv(CPUSPARCState *env, target_ulong a,
                            target_ulong b, int cc, uintptr_t ra)
{
    int overflow = 0;
    uint64_t x0;
    uint32_t x1;

    x0 = (a & 0xffffffff) | ((int64_t) (env->y) << 32);
    x1 = (b & 0xffffffff);

    if (x1 == 0) {
        cpu_raise_exception_ra(env, TT_DIV_ZERO, ra);
    }

    x0 = x0 / x1;
    if (x0 > UINT32_MAX) {
        x0 = UINT32_MAX;
        overflow = 1;
    }

    if (cc) {
        env->cc_dst = x0;
        env->cc_src2 = overflow;
        env->cc_op = CC_OP_DIV;
    }
    return x0;
}

target_ulong helper_udiv(CPUSPARCState *env, target_ulong a, target_ulong b)
{
    return do_udiv(env, a, b, 0, GETPC());
}

target_ulong helper_udiv_cc(CPUSPARCState *env, target_ulong a, target_ulong b)
{
    return do_udiv(env, a, b, 1, GETPC());
}

static target_ulong do_sdiv(CPUSPARCState *env, target_ulong a,
                            target_ulong b, int cc, uintptr_t ra)
{
    int overflow = 0;
    int64_t x0;
    int32_t x1;

    x0 = (a & 0xffffffff) | ((int64_t) (env->y) << 32);
    x1 = (b & 0xffffffff);

    if (x1 == 0) {
        cpu_raise_exception_ra(env, TT_DIV_ZERO, ra);
    } else if (x1 == -1 && x0 == INT64_MIN) {
        x0 = INT32_MAX;
        overflow = 1;
    } else {
        x0 = x0 / x1;
        if ((int32_t) x0 != x0) {
            x0 = x0 < 0 ? INT32_MIN : INT32_MAX;
            overflow = 1;
        }
    }

    if (cc) {
        env->cc_dst = x0;
        env->cc_src2 = overflow;
        env->cc_op = CC_OP_DIV;
    }
    return x0;
}

target_ulong helper_sdiv(CPUSPARCState *env, target_ulong a, target_ulong b)
{
    return do_sdiv(env, a, b, 0, GETPC());
}

target_ulong helper_sdiv_cc(CPUSPARCState *env, target_ulong a, target_ulong b)
{
    return do_sdiv(env, a, b, 1, GETPC());
}

#ifdef TARGET_SPARC64
int64_t helper_sdivx(CPUSPARCState *env, int64_t a, int64_t b)
{
    if (b == 0) {
        /* Raise divide by zero trap.  */
        cpu_raise_exception_ra(env, TT_DIV_ZERO, GETPC());
    } else if (b == -1) {
        /* Avoid overflow trap with i386 divide insn.  */
        return -a;
    } else {
        return a / b;
    }
}

uint64_t helper_udivx(CPUSPARCState *env, uint64_t a, uint64_t b)
{
    if (b == 0) {
        /* Raise divide by zero trap.  */
        cpu_raise_exception_ra(env, TT_DIV_ZERO, GETPC());
    }
    return a / b;
}
#endif

target_ulong helper_taddcctv(CPUSPARCState *env, target_ulong src1,
                             target_ulong src2)
{
    target_ulong dst;

    /* Tag overflow occurs if either input has bits 0 or 1 set.  */
    if ((src1 | src2) & 3) {
        goto tag_overflow;
    }

    dst = src1 + src2;

    /* Tag overflow occurs if the addition overflows.  */
    if (~(src1 ^ src2) & (src1 ^ dst) & (1u << 31)) {
        goto tag_overflow;
    }

    /* Only modify the CC after any exceptions have been generated.  */
    env->cc_op = CC_OP_TADDTV;
    env->cc_src = src1;
    env->cc_src2 = src2;
    env->cc_dst = dst;
    return dst;

 tag_overflow:
    cpu_raise_exception_ra(env, TT_TOVF, GETPC());
}

target_ulong helper_tsubcctv(CPUSPARCState *env, target_ulong src1,
                             target_ulong src2)
{
    target_ulong dst;

    /* Tag overflow occurs if either input has bits 0 or 1 set.  */
    if ((src1 | src2) & 3) {
        goto tag_overflow;
    }

    dst = src1 - src2;

    /* Tag overflow occurs if the subtraction overflows.  */
    if ((src1 ^ src2) & (src1 ^ dst) & (1u << 31)) {
        goto tag_overflow;
    }

    /* Only modify the CC after any exceptions have been generated.  */
    env->cc_op = CC_OP_TSUBTV;
    env->cc_src = src1;
    env->cc_src2 = src2;
    env->cc_dst = dst;
    return dst;

 tag_overflow:
    cpu_raise_exception_ra(env, TT_TOVF, GETPC());
}

#ifndef TARGET_SPARC64
void helper_power_down(CPUSPARCState *env)
{
    CPUState *cs = CPU(sparc_env_get_cpu(env));

    cs->halted = 1;
    cs->exception_index = EXCP_HLT;
    env->pc = env->npc;
    env->npc = env->pc + 4;
    cpu_loop_exit(cs);
}
#endif

#ifdef CONFIG_FLEXUS

#include <string.h>

#define FSR_FCC2_SHIFT 12
#define FSR_FCC2 (1ULL << FSR_FCC2_SHIFT)
//and FCC3
#define FSR_FCC3_SHIFT 13
#define FSR_FCC3 (1ULL << FSR_FCC3_SHIFT)

#define FSR_FTT2   (1ULL << 16)                                             
#define FSR_FTT1   (1ULL << 15)
#define FSR_FTT0   (1ULL << 14)  
#define FSR_FTT_MASK (FSR_FTT2 | FSR_FTT1 | FSR_FTT0)

void flexus_transaction(CPUSPARCState *env, target_ulong vaddr, 
		 target_ulong paddr, target_ulong pc, mem_op_type_t type,
		 int size, int atomic, int asi, int prefetch_fcn, int io, uint8_t cache_bits);

int cpu_proc_num(void *cs_) {
  CPUState *cs = (CPUState*)cs_;
  return cs->cpu_index;
}

void cpu_pop_indexes(int *indexes) {
	int i = 0;
	CPUState *cpu;
	CPU_FOREACH(cpu) {
		indexes[i] = cpu->cpu_index;
		i++;
	}
}

uint64_t cpu_get_program_counter(void *cs_) {
  CPUState *cs = (CPUState*)cs_;
  CPUSPARCState *env_ptr = cs->env_ptr;
  return (env_ptr->pc);
}

void *cpu_get_address_space_flexus(void *cs_) {
CPUState *cs = (CPUState*)cs_;
	return cs->as;
}

physical_address_t mmu_logical_to_physical(void *cs_, logical_address_t va) {
  CPUState *cs = (CPUState*)cs_;
	physical_address_t pa = cpu_get_phys_page_debug(cs, (va & TARGET_PAGE_MASK));
	// assuming phys address and logical address are the right size
    // this gets the page then we need to do get the place in the page using va
    // logical_address_t mask = 0x0000000000000FFF; 
	// The offset seems to be 12bits for 32bit or 64bit addresses
    pa = pa + (va & ~TARGET_PAGE_MASK);
	return pa;
}

uint64_t readReg(void *cs_, int reg_idx, int reg_type) {
#if defined(TARGET_SPARC64)
	CPUState *cs = (CPUState*)cs_;
	CPUSPARCState *env_ptr = cs->env_ptr;
	switch (reg_type) {
	case 0:	//GREGS
		assert(reg_idx < 8);
		return  env_ptr->gregs[reg_idx];
	case 1: //AGREGS
		assert(reg_idx < 8);
		return  env_ptr->agregs[reg_idx];
	case 2: //IGREGS
		assert(reg_idx < 8);
		return  env_ptr->igregs[reg_idx];
	case 3: //MGREGS
		assert(reg_idx < 8);
		return  env_ptr->mgregs[reg_idx];
	case 4: //windowed regs
		//ALEX - I *think* these are the register windows
		assert(reg_idx >= 0);
		return env_ptr->regbase[reg_idx];
	case 5: //FP registers
		assert(reg_idx < 32);
		return env_ptr->fpr[reg_idx].ll; 
	default:
		assert(false);
	}
#else
	assert(false);	//Currently only implemented for sparc64 
#endif
}

// prototype to suppress warning
uint64_t cpu_read_register_impl(void *cs_, int reg_index);
#ifdef CONFIG_SAIVASH
//PSTATE
//11 10  9  8  7  6  5  4  3  2  1  0 
//IG MG                            AG
#define AG_BIT 1 << 0
#define MG_BIT 1 << 10
#define IG_BIT 1 << 11
#endif // CONFIG_SAIVASH
uint64_t cpu_read_register_impl(void *cs_, int reg_index) {
	CPUState *cs = (CPUState*)cs_;
	CPUSPARCState *env_ptr = cs->env_ptr;

#ifdef CONFIG_SAIVASH
	switch(reg_index){
	case 0 ... 7://Global registers
		if (env_ptr->pstate & AG_BIT) //If AG is enable we should read from alternater global registers
			return env_ptr->agregs[reg_index];
		if (env_ptr->pstate & IG_BIT)//If IG is enable we should read from interrup global registers
			return env_ptr->igregs[reg_index];
		if (env_ptr->pstate & MG_BIT)//If MG is enable we should read from mmu global registers. TODO Check whether we should read from mgreg or mmureg
			return env_ptr->mgregs[reg_index];
		return env_ptr->gregs[reg_index];
	case 8 ... 31://Windowed registers
		return env_ptr->regwptr[reg_index - 8];//TODO Check whether we should read from regbase or regwptr	
	case 34:// Y
		return (env_ptr->y);
	case 35://CCR
		return cpu_get_ccr(env_ptr);
	case 36://FPRS
		return env_ptr->fprs;
	case 37://FSR
		return env_ptr->fsr;
	case 38://ASI
		return env_ptr->asi;
	case 39://TICK
		return env_ptr->tick;
	case 40://GSR
		return env_ptr->gsr;
	case 43://STICK
		return env_ptr->stick;
	case 45://PSTATE
		return env_ptr->pstate;
	case 46://TL
		return env_ptr->tl;
	case 47://PIL
		return env_ptr->psrpil;
	case 48 ... 57://TCP
		return env_ptr->ts[reg_index - 48].tpc;
	case 58 ... 67://TNPC
		return env_ptr->ts[reg_index - 58].tnpc;
	case 68 ... 77://TSTATE
		return env_ptr->ts[reg_index - 68].tstate;
	case 78 ... 87://TT
		return env_ptr->ts[reg_index - 78].tt;
	case 88://TBA -> TBR
		return env_ptr->tbr;
	case 89://VER
		return env_ptr->version;
	case 90://CWP
		return cpu_get_cwp64(env_ptr);
	case 91://CANSAVE
		return env_ptr->cansave;
	case 92://CANRESTORE
		return env_ptr->canrestore;
	case 93://OTHERWIN
		return env_ptr->otherwin;
	case 94://WSTATE
		return env_ptr->wstate;
	case 95://CLEANWIN
		return env_ptr->cleanwin;
	case 102://NPC -> FIXME
		return env_ptr->npc;		
	default:
		printf("Invalid Register :{%d}\n", reg_index);
		assert(0);
    	}
#endif // CONFIG_SAIVASH
if(reg_index<8){//general registers
        return env_ptr->gregs[reg_index];
    }else if(reg_index<32){//register window regs
        return env_ptr->regwptr[reg_index-8];//Register window registers(i,o,l regs)
    }else if(reg_index<96){//floating point regs
        //since there are only 32 FP regs in qemu and they are each defined as a double, union two 32bit ints
        //I think that it might be that fp reg 0 will be reg 32 and reg 33  
        //fpr
        if(reg_index % 2){//the register is odd
            //Possibly should make a new variable for clarity.
            //reg_index/2 because each register number is being counted twice(fp 0 is reg_index 0_lower(or upper?) and reg_index 1 is fp 0 upper(or lower)
            //- 16 is because 32/2 = 16 and we need to subtract -32 since we are starting the fp regs at reg_index 32 instead of 0.
            return (env_ptr->fpr[reg_index/2-16]).l.lower;//lower based on monitor.c:~4000(search for "fpr[0].l.upper
        }else{
            return (env_ptr->fpr[reg_index/2-16]).l.upper;//upper?
        } 
    }else{//all specific cases now
        switch(reg_index){
        case 96://FCC0
            return (env_ptr->fsr & FSR_FCC0); //not sure what it is in qemusparc
            break;
        case 97://FCC1
            return (env_ptr->fsr & FSR_FCC1); //not sure what it is in qemusparc
    
            //This is what they are for FCC0 and 1
            //define FSR_FCC1_SHIFT 11
            //define FSR_FCC1  (1ULL << FSR_FCC1_SHIFT)
            //define FSR_FCC0_SHIFT 10
            //define FSR_FCC0  (1ULL << FSR_FCC0_SHIFT)

            //so FCC2 would be 
            //define FSR_FCC2_SHIFT 100
            //define FSR_FCC2 (1ULL << FSR_FCC_2_SHIFT)
            //and FCC3
            //define FSR_FCC3_SHIFT 101
            //define FSR_FCC3 (1ULL << FSR_FCC_3_SHIFT)
            break;
        case 98://FCC2
            return (env_ptr->fsr & FSR_FCC2);
            break;
        case 99://FCC3
            return (env_ptr->fsr & FSR_FCC3);//FIXME no idea if these work or how to test
            break;
        case 100://CC
            //return env_ptr-> //might be harder to do since qemu does lazy cc codes.
            return cpu_get_ccr(env_ptr);//not sure if this is right(ccr might not be cc);
            break;
        case 101://PC
            return env_ptr->pc; 
            break;
        case 102://NPC
            return env_ptr->npc; 
            break;
        case 103://AEXC --part of fsr
            return (env_ptr->fsr & FSR_AEXC_MASK); 
            break;
        case 104://CEXC --part of fsr
            return (env_ptr->fsr & FSR_CEXC_MASK); 
            break;
        case 105://FTT --part of fsr
            return (env_ptr->fsr & FSR_FTT_MASK); //isn't defined for some reason, will have to look more at target-sparc/cpu.h:183
            break;
        case 106://DUDL --part of fprs
            //return (env_ptr->fsr & FSR_FTT_MASK);//haven't even found anything about the DUDL reg with googling.
            break;
        case 107://FEF --part of fprs
            return (env_ptr->fsr & FPRS_FEF);
            break;
        case 108://Y
            return (env_ptr->y);
            break;
        case 109://GSR
            return (env_ptr->gsr);
            break;
        case 110://CANSAVE
            return (env_ptr->cansave);
            break;
        case 111://CANRESTORE
            return (env_ptr->canrestore);
            break;
        case 112://OTHERWIN
            return (env_ptr->otherwin);
            break;
        case 113://CLEANWIN
            return (env_ptr->cleanwin);
            break;
        case 114://CWP
            //sounds specific to sparc64, do we care about 32bit sparc?
            return cpu_get_cwp64(env_ptr);//seems to actually work
            //there is no cpu_get_cwp(env). But I don't think it matters.
            break;
        case 115://ASI
            return (env_ptr->asi);
            break;
        //116-123 arent in the simics thing so they probably aren't used?
        //THESE MIGHT BE SIMICS SPECIFIC?
        case 124://Not_Used --not sure what this means 
            //return (env_ptr->asi);
            break;
        case 125://Sync --not sure what this means
            //return (env_ptr->asi);
            break;
#ifdef CONFIG_SAIVASH
        case 126://NOOSHIN: PSTATE
            return (env_ptr->pstate);
            break;
#endif
        } 
    }
	printf("WARNING: No register found, returning dummy register value\n");
	return -1;
}

void cpu_read_register( void *env_ptr, int reg_index, unsigned *reg_size, void *data_out ) {
  if( reg_size != NULL )
    *reg_size = 8;

  if( data_out != NULL ) {
    uint64_t reg_content = cpu_read_register_impl(env_ptr, reg_index);
    memcpy( data_out, &reg_content, sizeof(__uint64_t) );
  }
}
#ifdef CONFIG_SAIVASH


//SIA
void cpu_write_register_impl(void *cs_, int reg_index, uint64_t value);

void cpu_write_register_impl(void *cs_, int reg_index, uint64_t value){
       CPUState *cs = (CPUState*)cs_;
        CPUSPARCState *env_ptr = cs->env_ptr;

        if(reg_index<8){//general registers
        	env_ptr->gregs[reg_index] = value;
    }else if(reg_index<32){//register window regs
        env_ptr->regwptr[reg_index-8] = value;//Register window registers(i,o,l regs)
    }else if(reg_index<96){//floating point regs
        //since there are only 32 FP regs in qemu and they are each defined as a double, union two 32bit ints
        //I think that it might be that fp reg 0 will be reg 32 and reg 33
        //fpr
        if(reg_index % 2){//the register is odd
            //Possibly should make a new variable for clarity.
            //reg_index/2 because each register number is being counted twice(fp 0 is reg_index 0_lower(or upper?) and reg_index 1 is fp 0 upper(or lower)
            //- 16 is because 32/2 = 16 and we need to subtract -32 since we are starting the fp regs at reg_index 32 instead of 0.
            (env_ptr->fpr[reg_index/2-16]).l.lower = value;//lower based on monitor.c:~4000(search for "fpr[0].l.upper
        }else{
            (env_ptr->fpr[reg_index/2-16]).l.upper = value;//upper?
        }

    }else{//all specific cases now
        switch(reg_index){
        case 96://FCC0
            if(value)
            	env_ptr->fsr |= FSR_FCC0; //not sure what it is in qemusparc
            break;
        case 97://FCC1
	    
            if(value)
            env_ptr->fsr |=  FSR_FCC1; //not sure what it is in qemusparc

            //This is what they are for FCC0 and 1
            //define FSR_FCC1_SHIFT 11
            //define FSR_FCC1  (1ULL << FSR_FCC1_SHIFT)
            //define FSR_FCC0_SHIFT 10
            //define FSR_FCC0  (1ULL << FSR_FCC0_SHIFT)

            //so FCC2 would be
            //define FSR_FCC2_SHIFT 100
            //define FSR_FCC2 (1ULL << FSR_FCC_2_SHIFT)
            //and FCC3
            //define FSR_FCC3_SHIFT 101
            //define FSR_FCC3 (1ULL << FSR_FCC_3_SHIFT)
            break;
        case 98://FCC2
            if(value)
            env_ptr->fsr |=  FSR_FCC2;
            break;
        case 99://FCC3
            if(value)
            env_ptr->fsr |=  FSR_FCC3;//FIXME no idea if these work or how to test
            break;
        case 100://CC
            //return env_ptr-> //might be harder to do since qemu does lazy cc codes.
            cpu_put_ccr(env_ptr, value);//not sure if this is right(ccr might not be cc);
            break;
        case 101://PC
            env_ptr->pc = value;
            break;
        case 102://NPC
            //printf("QEMU: getting NPC from QEMU: %x",env_ptr->npc);//NOOSHIN
            env_ptr->npc = value;
            break;
        case 103://AEXC --part of fsr
            if(value)
            env_ptr->fsr |= FSR_AEXC_MASK;
            break;
        case 104://CEXC --part of fsr
            if(value)
            env_ptr->fsr |= FSR_CEXC_MASK;
            break;
        case 105://FTT --part of fsr
            if(value)
            env_ptr->fsr |= FSR_FTT_MASK; //isn't defined for some reason, will have to look more at target-sparc/cpu.h:183
            break;
        case 106://DUDL --part of fprs
            //return (env_ptr->fsr & FSR_FTT_MASK);//haven't even found anything about the DUDL reg with googling.
            break;
        case 107://FEF --part of fprs
            if(value)
            env_ptr->fsr |= FPRS_FEF;
            break;
        case 108://Y
            (env_ptr->y) = value;
            break;
        case 109://GSR
            (env_ptr->gsr) = value;
            break;
        case 110://CANSAVE
            (env_ptr->cansave) = value;
            break;
        case 111://CANRESTORE
            (env_ptr->canrestore) = value;
            break;
        case 112://OTHERWIN
            (env_ptr->otherwin) = value;
            break;
        case 113://CLEANWIN
            (env_ptr->cleanwin) = value;
            break;
        case 114://CWP
            //sounds specific to sparc64, do we care about 32bit sparc?
            cpu_put_cwp64(env_ptr, value);//seems to actually work
            //there is no cpu_get_cwp(env). But I don't think it matters.
            break;
        case 115://ASI
            (env_ptr->asi) = value;
            break;
        //116-123 arent in the simics thing so they probably aren't used?
        //THESE MIGHT BE SIMICS SPECIFIC?
        case 124://Not_Used --not sure what this means
            //return (env_ptr->asi);
            break;
        case 125://Sync --not sure what this means
            //return (env_ptr->asi);
            break;
       case 126://NOOSHIN: PSTATE
            (env_ptr->pstate) = value;
            break;
        }
    }
}

void cpu_write_register( void *env_ptr, int reg_index, unsigned *reg_size, uint64_t value ) {
  if( reg_size != NULL )
    *reg_size = 8;
  cpu_write_register_impl(env_ptr, reg_index, value);
  
}
//End SIA
#endif
/* cached variables */
conf_object_t space_cached;
memory_transaction_t mem_trans_cached;
QEMU_callback_args_t event_data_cached;
QEMU_ncm ncm_cached;

void helper_flexus_magic_ins(int v){
  switch(v) {
    case 42:
      printf("Toggling simulation on!\n");
      QEMU_toggle_simulation(1);
      break;
    case 43:
      printf("Toggling simulation off!\n");
      QEMU_toggle_simulation(0);
      break;
    case 44:
      QEMU_break_simulation("Magic instruction caused the end of the simulation.");
      break;
    default:
      //printf("Received magic instruction: %d\n", v);
      break;
    };
}

void helper_flexus_periodic(CPUSPARCState *env){
  SPARCCPU *sparc_cpu = sparc_env_get_cpu(env);
  CPUState *cpu = CPU(sparc_cpu);
  QEMU_increment_instruction_count(cpu_proc_num(cpu), 2);

  static uint64_t instCnt = 0;

  int64_t simulation_length = QEMU_get_simulation_length();
  if( simulation_length >= 0 && instCnt >= simulation_length ) {

    static int already_tried_to_exit = 0;

    if( already_tried_to_exit == 0 ) {
      already_tried_to_exit = 1;
      QEMU_break_simulation("Reached the end of the simulation");
    }

 //   exit_request = 1;
    cpu->exit_request = 1;
    cpu_loop_exit(cpu);

    return ;
  }

  instCnt++;
  QEMU_increment_instruction_count(cpu_proc_num(cpu), 2);

  uint64_t eventDelay = 1000;
  if((instCnt % eventDelay) ==0 ){
    QEMU_callback_args_t * event_data = &event_data_cached;
    event_data->ncm = &ncm_cached;
    QEMU_execute_callbacks(QEMUFLEX_GENERIC_CALLBACK, QEMU_periodic_event, event_data);
  }
}

/*
  FIXME:
    Someone should check the following helpers:
      helper_flexus_ld, helper_flexus_ld_asi, helper_flexus_ldf_asi, helper_flexus_ldda_asi,
      helper_flexus_st, etc...
      Basically, we need to check the whole sparc backend.
 */

void helper_flexus_ld(CPUSPARCState *env, target_ulong addr, target_ulong pc, int size, int mmu_idx, int is_atomic)
{
    int index = (addr >> TARGET_PAGE_BITS) & (CPU_TLB_SIZE - 1);
    target_ulong tlb_addr = env->tlb_table[mmu_idx][index].addr_read;

    if ((addr & TARGET_PAGE_MASK)
        != (tlb_addr & (TARGET_PAGE_MASK | TLB_INVALID_MASK))) {
        // Given that a previous load instruction happened, we are sure that
        // that the TLB entry is still in the CPU TLB, if not then the previous
        // instruction caused an error, so we just return with no tlb_fill() call
	return;
    }

    if ((addr & (size - 1)) != 0) {
	// Illegal memory address: not size-aligned 
        return;
    }	

    int asi;
    // Non-alternate-space Loads (SPARC V9 documention p.73)
    if(env->tl == 0){
       // Trap Level = 0
       if(env->pstate & PS_CLE){
       	       // ASI_PRIMARY_LITTLE
	       asi = 0x88;
       } else {
	       // ASI_PRIMARY 0x80
	       asi = 0x80;
       }
    } else {
       // Trap Level > 0
       if(env->pstate & PS_CLE){
	       // ASI_NUCLEUS_LITTLE 
	       asi = 0x0c;
       } else {
	       // ASI_NUCLEUS
	       asi = 0x04;
       }	
    }    
    
    target_ulong phys_address;
    // Getting page physical address
    phys_address = env->tlb_table[mmu_idx][index].paddr;
    // Resolving physical address by adding offset inside the page
    phys_address += addr & ~TARGET_PAGE_MASK;

    uint8_t cache_bits = env->tlb_table[mmu_idx][index].dummy[0];

    int io = 0;
    if (unlikely(tlb_addr & ~TARGET_PAGE_MASK)) {
	// I/O space
	io = 1;
    }        
    // Otherwise, RAM/ROM , physical memory space, io = 0
    
    // Here, prefetch_fcn is just a dummy argument since type is not prefetch
    flexus_transaction(env, addr, phys_address, pc, QEMU_Trans_Load,
		           size, is_atomic, asi, 0, io, cache_bits);   
}

void helper_flexus_ld_asi(CPUSPARCState *env, target_ulong addr, target_ulong pc, int size, int asi, int is_atomic)
{

    // This function is taken from helper_ld_asi in ldst_helper.c
    SPARCCPU *cpu = sparc_env_get_cpu(env);
    CPUState *cs = CPU(cpu);

    asi &= 0xff;

    // XXX: I don't know whether to include the second
    // check in helper_flexus_stf_asi, 
    //        (cpu_has_hypervisor(env)
    //        && asi >= 0x30 && asi < 0x80
    //        && !(env->hpstate & HS_PRIV))
    // since all ASIs there are 'and'ed
    // with either 0x8f or 0x19 when
    // they get called in to this function
    // so they become outside the range 0x30 - 0x80
    if ((asi < 0x80 && (env->pstate & PS_PRIV) == 0)
        || (cpu_has_hypervisor(env)
            && asi >= 0x30 && asi < 0x80
            && !(env->hpstate & HS_PRIV))) {
    	// Exception: privileged_action
        return;
    }

    if ((addr & (size - 1)) != 0) {
	// Illegal memory address: not aligned 
        return;
    }
    
    // MMU index needed to access the correct
    // entry in the softmmu TLB
    int mmu_idx = 0;

    // process nonfaulting loads first
    if ((asi & 0xf6) == 0x82) {
        // secondary space access has lowest asi bit equal to 1
        if (env->pstate & PS_PRIV) {
            mmu_idx = (asi & 1) ? MMU_KERNEL_SECONDARY_IDX : MMU_KERNEL_IDX;
        } else {
            mmu_idx = (asi & 1) ? MMU_USER_SECONDARY_IDX : MMU_USER_IDX;
        }

        if (cpu_get_phys_page_nofault(env, addr, mmu_idx) == -1ULL) {
		// An error occurred while translating
		// Since this is a non-faulting load, then just return
		return;
        }
    }

    // Check conformity of QEMU ASIs with Flexus ASIs
    switch (asi) {
    case 0x10: // As if user primary 
    case 0x11: // As if user secondary 
    case 0x18: // As if user primary LE 
    case 0x19: // As if user secondary LE 
    case 0x80: // Primary 
    case 0x81: // Secondary 
    case 0x88: // Primary LE 
    case 0x89: // Secondary LE 
    case 0xe2: // UA2007 Primary block init 
    case 0xe3: // UA2007 Secondary block init
    case 0x82: // Primary, no fault
    case 0x83: // Secondary, no fault
    case 0x8a: // Primary, no fault, LE
    case 0x8b: // Secondary, no fault, LE
    // Note: that cases 0x82,83,8a,8b have asi
    // such that (asi & 0xf6) = 0x82
    // In function, helper_ld_asi, these asi's 
    // are then 'and'ed with ~0x02, which 
    // leads to asi's 80,81,88,89 respectively
    // so we treat them the same way
        if ((asi & 0x80) && (env->pstate & PS_PRIV)) {
            if (cpu_hypervisor_mode(env)) {
		// Hypervisor mode
                mmu_idx = 5;
            } else {
                // secondary space access has lowest asi bit equal to 1 
                if (asi & 1) {
		    // Kernel secondary mode
                    mmu_idx = 3;
                } else {
		    // Kernel mode
                    mmu_idx = 2;
                }
            }
        } else {
            // secondary space access has lowest asi bit equal to 1 
            if (asi & 1) {
		// User secondary mode
                mmu_idx = 1;
            } 
		// Else in user mode
		// mmu_idx = 0, initialized when mmu_idx is declared
        }
        break;
    // Note: these are implementation-dependant ASIs
    case 0x14: // Bypass 
    case 0x15: // Bypass, non-cacheable 
    case 0x1c: // Bypass LE 
    case 0x1d: // Bypass, non-cacheable LE 
        {
	    // These ASIs are not translated by
    	    // the MMU; instead, they pass through
	    // their virtual addresses as physical addresses
	
    	    MemoryRegion *mr;
	    hwaddr addr1, l = size;
            mr = address_space_translate(cs->as, addr, &addr1, &l,
                                 false);
	    int is_io = 1;
	    if (memory_region_is_ram(mr) || memory_region_is_romd(mr)){
		// RAM/ROM case
		is_io = 0;
	    }
		// Otherwise I/O case, is_io initialized to 1

            int cacheBits = 3; // CP, CV set to 1
	    if(asi & 1){
		cacheBits = 0; // if non-cacheable asi set both CP,CV to 0
	    }

            // Cache Physical and Cache Virtual are both set to 1 if cacheable
	    // otherwise cleared to 0 if we have a non-cacheable asi
	    // This is a dummy assignment since there is no TLB entry involved
	    // Since it is a bypass asi
            // Here, prefetch_fcn is just a dummy argument since type is not prefetch
	    flexus_transaction(env, addr, addr, pc, QEMU_Trans_Load,
				 size, is_atomic, asi, 0, is_io, cacheBits);  

	    return;
        }
    case 0x04: // Nucleus 
    case 0x0c: // Nucleus Little Endian (LE) 
        {
            mmu_idx = 4;
	    break;
        }
    default:
        //no idea why it is return not break here.
       // return;
       break;
    }

    int index = (addr >> TARGET_PAGE_BITS) & (CPU_TLB_SIZE - 1);
    target_ulong tlb_addr = env->tlb_table[mmu_idx][index].addr_read;

    if ((addr & TARGET_PAGE_MASK)
        != (tlb_addr & (TARGET_PAGE_MASK | TLB_INVALID_MASK))) {
	// Given that a previous load instruction happened, we are sure that
        // that the TLB entry is still in the CPU TLB, if not then the previous
        // instruction caused an error, so we just return with no tlb_fill() call
	return;
    }

    target_ulong phys_address;
    // Getting page physical address
    phys_address = env->tlb_table[mmu_idx][index].paddr;
    // Resolving physical address by adding offset inside the page
    phys_address += addr & ~TARGET_PAGE_MASK;

    uint8_t cache_bits = env->tlb_table[mmu_idx][index].dummy[0];

    int io = 0;
    if (unlikely(tlb_addr & ~TARGET_PAGE_MASK)) {
	// I/O space
	io = 1;
    }        
    // Otherwise, RAM/ROM , physical memory space, io = 0

    // Here, prefetch_fcn is just a dummy argument since type is not prefetch
    flexus_transaction(env, addr, phys_address, pc, QEMU_Trans_Load,
		 	   size, is_atomic, asi, 0, io, cache_bits);   
}

void helper_flexus_ldf_asi(CPUSPARCState *env, target_ulong addr, target_ulong pc, int size,  int asi, int rd)
{
    // This code is taken from helper_ldf_asi in ldst_helper.c
    if ((addr & 3) != 0) {
	// Illegal memory address: not word aligned 
	// Floating point operations have a minimum size of 4
	// Therefore, the address must be atleast 4-byte aligned
        return;
    }

    // MMU index needed to access the correct
    // entry in the softmmu TLB
    int mmu_idx = 0;
    // Privileged mode: initially false
    //bool priv = false;
    switch (asi) {
    // asi & 0x8F -> asi in helper_ld_asi 
    case 0xf0: // UA2007/JPS1 Block load primary -> 80
    case 0xf1: // UA2007/JPS1 Block load secondary -> 81
    case 0xf8: // UA2007/JPS1 Block load primary LE -> 88
    case 0xf9: // UA2007/JPS1 Block load secondary LE -> 89
        if (rd & 7) {
            // Illegal instruction
	    // The floating-point unit is not enabled for these source registers
            return;
        }

        // Checking alignement
	if ((addr & 63) != 0) {
		// Illegal memory address: not 64-byte aligned 
		// Block stores must be 64-byte aligned
        	return;
        }
	
	if (env->pstate & PS_PRIV) {
	  //priv = true;
            if (cpu_hypervisor_mode(env)) {
		// Hypervisor mode
                mmu_idx = 5;
            } else {
                // secondary space access has lowest asi bit equal to 1 
                if (asi & 1) {
		    // Kernel secondary mode
                    mmu_idx = 3;
                } else {
		    // Kernel mode
                    mmu_idx = 2;
                }
            }
        } else {
            // secondary space access has lowest asi bit equal to 1 
            if (asi & 1) {
		// User secondary mode
                mmu_idx = 1;
            } 
		// Else in user mode
		// mmu_idx = 0, initialized when mmu_idx is declared
        }

	goto do_store;

    // asi & 0x19 -> asi in helper_ld_asi
    case 0x16: // UA2007 Block load primary, user privilege -> 10
    case 0x17: // UA2007 Block load secondary, user privilege -> 11
    case 0x1e: // UA2007 Block load primary LE, user privilege -> 18
    case 0x1f: // UA2007 Block load secondary LE, user privilege -> 19
    case 0x70: // JPS1 Block load primary, user privilege -> 10
    case 0x71: // JPS1 Block load secondary, user privilege -> 11
    case 0x78: // JPS1 Block load primary LE, user privilege -> 18
    case 0x79: // JPS1 Block load secondary LE, user privilege -> 19
        if (rd & 7) {
            // Illegal instruction
	    // The floating-point unit is not enabled for these source registers
            return;
        }
        // Checking alignement
	if ((addr & 63) != 0) {
		// Illegal memory address: not 64-byte aligned 
		// Block stores must be 64-byte aligned
        	return;
        }
	//priv = true;
	if((env->pstate & PS_PRIV) == 0){
	// ASIs 0x00 to 0x7F are restricted, only priveleged software is allowed to access them
	// If PSTATE.PRIV bit = 0, then we are in user mode -> exception
		return;
	}
	
	// Here, we are definately accessing user space
	// but we need to figure out whether
	// it is secondary or primary
	// secondary space access has lowest asi bit equal to 1 
        if (asi & 1) {
		// User secondary
                mmu_idx = 1;
        } 
		// Else User MMU_idx
		// mmu_idx = 0, initialized when mmu_idx is declared
    do_store: ;
	int index = (addr >> TARGET_PAGE_BITS) & (CPU_TLB_SIZE - 1);
    	target_ulong tlb_addr = env->tlb_table[mmu_idx][index].addr_read;

    	if ((addr & TARGET_PAGE_MASK)
        	!= (tlb_addr & (TARGET_PAGE_MASK | TLB_INVALID_MASK))) {
		// Given that a previous load instruction happened, we are sure that
        	// that the TLB entry is still in the CPU TLB, if not then the previous
        	// instruction caused an error, so we just return with no tlb_fill() call
		return;
    	}

	target_ulong phys_address;
	// Getting page physical address
	phys_address = env->tlb_table[mmu_idx][index].paddr;
	// Resolving physical address by adding offset inside the page
	phys_address += addr & ~TARGET_PAGE_MASK;

    	uint8_t cache_bits = env->tlb_table[mmu_idx][index].dummy[0];

   	int io = 0;
    	if (unlikely(tlb_addr & ~TARGET_PAGE_MASK)) {
		// I/O space
		io = 1;
	    }        
	    // Otherwise, RAM/ROM , physical memory space, io = 0

    	// Here, prefetch_fcn is just a dummy argument since type is not prefetch
    	flexus_transaction(env, addr, phys_address, pc, QEMU_Trans_Load,
					 64, 0, asi, 0, io, cache_bits);   
        break;
    default:
        // Not atomic
	helper_flexus_ld_asi(env, addr, pc, size, asi, 0);
	break;
    }
}

void helper_flexus_ldda_asi(CPUSPARCState *env, target_ulong addr, target_ulong pc, int asi, int rd)
{
    // Taken from helper_ldda_asi in ldst_helper.c
    if ((asi < 0x80 && (env->pstate & PS_PRIV) == 0)
        || (cpu_has_hypervisor(env)
            && asi >= 0x30 && asi < 0x80
            && !(env->hpstate & HS_PRIV))) {
        // Exception: Privileged Action
    }
    int index;
    switch (asi) {
    case 0x24: // Nucleus quad LDD 128 bit atomic
    case 0x2c: // Nucleus quad LDD 128 bit atomic LE 
        if ((addr & 0xf) != 0) {
		// Illegal memory address: not 16-byte aligned 
        	return;
    	}
	// Note that a load doubleword with rd = 0 modifies
	// only r[1] (Sparc V9 Documentation p.181)
	// I'm not sure whether this means only 8 bytes
	// are loaded or 16 bytes are loaded and the first
	// 8 bytes are discarded (the second assumption is implemented)
	
	index = (addr >> TARGET_PAGE_BITS) & (CPU_TLB_SIZE - 1);
	// MMU_INDEX 4 is for nucleus
    	target_ulong tlb_addr = env->tlb_table[4][index].addr_read;

    	if ((addr & TARGET_PAGE_MASK)
        	!= (tlb_addr & (TARGET_PAGE_MASK | TLB_INVALID_MASK))) {
		// Given that a previous load instruction happened, we are sure that
        	// that the TLB entry is still in the CPU TLB, if not then the previous
        	// instruction caused an error, so we just return with no tlb_fill() call
		return;
    	}

	target_ulong phys_address;
	// Getting page physical address
	phys_address = env->tlb_table[4][index].paddr;
	// Resolving physical address by adding offset inside the page
	phys_address += addr & ~TARGET_PAGE_MASK;

    	uint8_t cache_bits = env->tlb_table[4][index].dummy[0];

    	int io = 0;
    	if (unlikely(tlb_addr & ~TARGET_PAGE_MASK)) {
		// I/O space
		io = 1;
	}        
    	// Otherwise, RAM/ROM , physical memory space, io = 0

   	// Here, QEMU_Trans_Load is just a dummy argument, not used because
    	// is_atomic == 1. So is prefetch_fcn, since type is not prefetch
    	flexus_transaction(env, addr, phys_address, pc, QEMU_Trans_Load,
					 16, 0, asi, 0, io, cache_bits);   
        break;
    default:
        if ((addr & 0x7) != 0) {
		// Illegal memory address: not 8-byte aligned 
        	return;
    	}
	// Note: we have the same problem as above when rd = 0
        // LDDA instructions are atomic according to Simics
        helper_flexus_ld_asi(env, addr, pc, 8, asi, 0);
        break;
    }
}

void helper_flexus_st(CPUSPARCState *env, target_ulong addr, target_ulong pc, int size, int mmu_idx, int is_atomic)
{ 

    int index = (addr >> TARGET_PAGE_BITS) & (CPU_TLB_SIZE - 1);
    target_ulong tlb_addr = env->tlb_table[mmu_idx][index].addr_write;

    if ((addr & TARGET_PAGE_MASK)
        != (tlb_addr & (TARGET_PAGE_MASK | TLB_INVALID_MASK))) {
	// Given that a previous load instruction happened, we are sure that
        // that the TLB entry is still in the CPU TLB, if not then the previous
        // instruction caused an error, so we just return with no tlb_fill() call
	return;
    }

    if ((addr & (size - 1)) != 0) {
	// Illegal memory address: not size-aligned 
        return;
    }

    target_ulong phys_address;
    // Getting page physical address
    phys_address = env->tlb_table[mmu_idx][index].paddr;
    // Resolving physical address by adding offset inside the page
    phys_address += addr & ~TARGET_PAGE_MASK;

    uint8_t cache_bits = env->tlb_table[mmu_idx][index].dummy[0];

    int asi;
    // PSTATE.PRIV bit indicates whether we are in privileged mode
    bool priv = (env->pstate & PS_PRIV);
    // Non-alternate-space Loads (SPARC V9 documention p.73)
    if(env->tl == 0){
       // Trap Level = 0
       if(env->pstate & PS_CLE){
       	       // ASI_PRIMARY_LITTLE 0x88
	       asi = 0x88;
       } else {
	       // ASI_PRIMARY 0x80
	       asi = 0x80;
       }
    } else {
       // Trap Level > 0
       if(priv == 0){
	       // TL > 0 and PSTATE.PRIV = 0 is invalid
       	       return;
       }
       if(env->pstate & PS_CLE){
	       // ASI_NUCLEUS_LITTLE 
	       asi = 0x0c;
       } else {
	       // ASI_NUCLEUS
	       asi = 0x04;
       }
    }    


    int io = 0;
    if (unlikely(tlb_addr & ~TARGET_PAGE_MASK)) {
	// I/O space
	io = 1;
    }
    // Otherwise, RAM/ROM , physical memory space, io = 0
    
    // Here, prefetch_fcn is just a dummy argument since type is not prefetch
    flexus_transaction(env, addr, phys_address, pc, QEMU_Trans_Store,
			    size, is_atomic, asi, 0, io, cache_bits);   

}

void helper_flexus_st_asi(CPUSPARCState *env, target_ulong addr, target_ulong pc, int size, int asi, int is_atomic)
{

    // This function is taken from helper_st_asi in ldst_helper.c
    SPARCCPU *cpu = sparc_env_get_cpu(env);
    CPUState *cs = CPU(cpu);

    asi &= 0xff;

    // XXX: I don't know whether to include the second
    // check in helper_flexus_stf_asi, since all ASI's
    // there are 'and'ed with either 0x8f or 0x19 and
    // therefore become outside the range 0x30 - 0x80
    if ((asi < 0x80 && (env->pstate & PS_PRIV) == 0)
        || (cpu_has_hypervisor(env)
            && asi >= 0x30 && asi < 0x80
            && !(env->hpstate & HS_PRIV))) {
    	// Exception: privileged_action
        return;
    }

    if ((addr & (size - 1)) != 0) {
	// Illegal memory address: not aligned 
        return;
    }
    
    // MMU index needed to access the correct
    // entry in the softmmu TLB
    int mmu_idx = 0;
    // Privileged mode: initially false
    //bool priv = false;
    // Check conformity of QEMU ASIs with Flexus ASIs
    switch (asi) {
    case 0x10: // As if user primary 
    case 0x11: // As if user secondary 
    case 0x18: // As if user primary LE 
    case 0x19: // As if user secondary LE 
    case 0x80: // Primary 
    case 0x81: // Secondary 
    case 0x88: // Primary LE 
    case 0x89: // Secondary LE 
    case 0xe2: // UA2007 Primary block init 
    case 0xe3: // UA2007 Secondary block init 
        if ((asi & 0x80) && (env->pstate & PS_PRIV)) {
	  //priv = true;
            if (cpu_hypervisor_mode(env)) {
		// Hypervisor mode
                mmu_idx = 5;
            } else {
                // secondary space access has lowest asi bit equal to 1 
                if (asi & 1) {
		    // Kernel secondary mode
                    mmu_idx = 3;
                } else {
		    // Kernel mode
                    mmu_idx = 2;
                }
            }
        } else {
            // secondary space access has lowest asi bit equal to 1 
            if (asi & 1) {
		// User secondary mode
                mmu_idx = 1;
            } 
		// Else in user mode
		// mmu_idx = 0, initialized when mmu_idx is declared
        }
        break;
    // Note: these are implementation-dependant ASIs
    case 0x14: // Bypass 
    case 0x15: // Bypass, non-cacheable 
    case 0x1c: // Bypass LE 
    case 0x1d: // Bypass, non-cacheable LE 
        {
	    // These ASIs are not translated by
    	    // the MMU; instead, they pass through
	    // their virtual addresses as physical addresses
	    //priv = true;
	
    	    MemoryRegion *mr;
	    hwaddr addr1, l = size;
            mr = address_space_translate(cs->as, addr, &addr1, &l,
                                 true);
	    int is_io = 1;
	    if (memory_region_is_ram(mr)){
		// RAM case
		if(mr->readonly){ 
			// Corrupted Memory Region ?
			return;
		}
		is_io = 0;
	    } else if(memory_region_is_romd(mr)){
		// ROM case
		// Return because we are trying to write 
		// to read-only memory
		return;
	    }
		// Otherwise I/O case, is_io initialized to 1

            int cacheBits = 3; // CP, CV set to 1
	    if(asi & 1){
		cacheBits = 0; // if non-cacheable asi set both CP,CV to 0
	    }

            // Cache Physical and Cache Virtual are both set to 1 if cacheable
	    // otherwise cleared to 0 if we have a non-cacheable asi
	    // This is a dummy assignment since there is no TLB entry involved
	    // Since it is a bypass asi
            // Here, prefetch_fcn is just a dummy argument since type is not prefetch
    	    flexus_transaction(env, addr, addr, pc, QEMU_Trans_Store,
				  size, is_atomic, asi, 0, is_io, cacheBits); 
	    return;
        }
    case 0x04: // Nucleus 
    case 0x0c: // Nucleus Little Endian (LE) 
        {
	  //priv = true;
            mmu_idx = 4;
	    break;
        }
    default:
        //break;
   //FIXME should probably break if the asi isn't recognized?     
        //break;
        return;
    }

    int index = (addr >> TARGET_PAGE_BITS) & (CPU_TLB_SIZE - 1);
    target_ulong tlb_addr = env->tlb_table[mmu_idx][index].addr_write;

    if ((addr & TARGET_PAGE_MASK)
        != (tlb_addr & (TARGET_PAGE_MASK | TLB_INVALID_MASK))) {
	// Given that a previous load instruction happened, we are sure that
        // that the TLB entry is still in the CPU TLB, if not then the previous
        // instruction caused an error, so we just return with no tlb_fill() call
        return;
    }

    target_ulong phys_address;
    // Getting page physical address
    phys_address = env->tlb_table[mmu_idx][index].paddr;
    // Resolving physical address by adding offset inside the page
    phys_address += addr & ~TARGET_PAGE_MASK;

    uint8_t cache_bits = env->tlb_table[mmu_idx][index].dummy[0];

    int io = 0;
    if (unlikely(tlb_addr & ~TARGET_PAGE_MASK)) {
	// I/O space
	io = 1;
    }        
    // Otherwise, RAM/ROM , physical memory space, io = 0

    // Here, prefetch_fcn is just a dummy argument since type is not prefetch
    flexus_transaction(env, addr, phys_address, pc, QEMU_Trans_Store,
			    size, is_atomic, asi, 0, io, cache_bits);   

}

void helper_flexus_stf_asi(CPUSPARCState *env, target_ulong addr, target_ulong pc, int size,  int asi, int rd)
{

    // This code is taken from helper_stf_asi in ldst_helper.c
    if ((addr & 3) != 0) {
	// Illegal memory address: not word aligned 
	// Floating point operations have a minimum size of 4
	// Therefore, the address must be atleast 4-byte aligned
        return;
    }

    // MMU index needed to access the correct
    // entry in the softmmu TLB
    int mmu_idx = 0;
    // Privileged mode: initially false
    //bool priv = false;
    switch (asi) {
    // asi & 0x8F -> asi in helper_st_asi 
    case 0xe0: // UA2007/JPS1 Block commit store primary (cache flush) -> 80
    case 0xe1: // UA2007/JPS1 Block commit store secondary (cache flush) -> 81
    case 0xf0: // UA2007/JPS1 Block store primary -> 80
    case 0xf1: // UA2007/JPS1 Block store secondary -> 81
    case 0xf8: // UA2007/JPS1 Block store primary LE -> 88
    case 0xf9: // UA2007/JPS1 Block store secondary LE -> 89
        if (rd & 7) {
            // Illegal instruction
	    // The floating-point unit is not enabled for these source registers
            return;
        }

        // Checking alignement
	if ((addr & 63) != 0) {
		// Illegal memory address: not 64-byte aligned 
		// Block stores must be 64-byte aligned
        	return;
        }
	
	if (env->pstate & PS_PRIV) {
	  //priv = true;
            if (cpu_hypervisor_mode(env)) {
		// Hypervisor mode
                mmu_idx = 5;
            } else {
                // secondary space access has lowest asi bit equal to 1 
                if (asi & 1) {
		    // Kernel secondary mode
                    mmu_idx = 3;
                } else {
		    // Kernel mode
                    mmu_idx = 2;
                }
            }
        } else {
            // secondary space access has lowest asi bit equal to 1 
            if (asi & 1) {
		// User secondary mode
                mmu_idx = 1;
            }
		// Else in user mode
		// mmu_idx = 0, initialized when mmu_idx is declared
        }

	goto do_store;

    // asi & 0x19 -> asi in helper_st_asi
    case 0x16: // UA2007 Block load primary, user privilege -> 10
    case 0x17: // UA2007 Block load secondary, user privilege -> 11
    case 0x1e: // UA2007 Block load primary LE, user privilege -> 18
    case 0x1f: // UA2007 Block load secondary LE, user privilege -> 19
    case 0x70: // JPS1 Block store primary, user privilege -> 10
    case 0x71: // JPS1 Block store secondary, user privilege -> 11
    case 0x78: // JPS1 Block load primary LE, user privilege -> 18
    case 0x79: // JPS1 Block load secondary LE, user privilege -> 19
        if (rd & 7) {
            // Illegal instruction
	    // The floating-point unit is not enabled for these source registers
            return;
        }
        // Checking alignement
	if ((addr & 63) != 0) {
		// Illegal memory address: not 64-byte aligned 
		// Block stores must be 64-byte aligned
        	return;
        }
	//priv = true;
	if((env->pstate & PS_PRIV) == 0){
	// ASIs 0x00 to 0x7F are restricted, only priveleged software is allowed to access them
	// If PSTATE.PRIV bit = 0, then we are in user mode -> exception
		return;
	}
	
	// Here, we are definately accessing user space
	// but we need to figure out whether
	// it is secondary or primary
	// secondary space access has lowest asi bit equal to 1 
        if (asi & 1) {
		// User secondary
                mmu_idx = 1;
        } 
		// Else User MMU_idx
		// mmu_idx = 0, initialized when mmu_idx is declared
    do_store: ;
	int index = (addr >> TARGET_PAGE_BITS) & (CPU_TLB_SIZE - 1);
    	target_ulong tlb_addr = env->tlb_table[mmu_idx][index].addr_write;

    	if ((addr & TARGET_PAGE_MASK)
        	!= (tlb_addr & (TARGET_PAGE_MASK | TLB_INVALID_MASK))) {
		// Given that a previous load instruction happened, we are sure that
        	// that the TLB entry is still in the CPU TLB, if not then the previous
        	// instruction caused an error, so we just return with no tlb_fill() call
		return;
    	}

	target_ulong phys_address;
	// Getting page physical address
	phys_address = env->tlb_table[mmu_idx][index].paddr;
	// Resolving physical address by adding offset inside the page
	phys_address += addr & ~TARGET_PAGE_MASK;

    	uint8_t cache_bits = env->tlb_table[mmu_idx][index].dummy[0];

    	int io = 0;
    	if (unlikely(tlb_addr & ~TARGET_PAGE_MASK)) {
		// I/O space
		io = 1;
    	}        
    	// Otherwise, RAM/ROM , physical memory space, io = 0
    
    	// Here, QEMU_Trans_Load is just a dummy argument, not used because
    	// is_atomic == 1. So is prefetch_fcn, since type is not prefetch
    	flexus_transaction(env, addr, phys_address, pc, QEMU_Trans_Store,
					  64, 0, asi, 0, io, cache_bits);   	
        break;
    default:
        // Not atomic
	helper_flexus_st_asi(env, addr, pc, size, asi, 0);
	break;
    }

}

void helper_flexus_rmw(CPUSPARCState *env, target_ulong addr, target_ulong pc, int size, int mmu_idx)
{
    int index = (addr >> TARGET_PAGE_BITS) & (CPU_TLB_SIZE - 1);
    target_ulong tlb_addr = env->tlb_table[mmu_idx][index].addr_write;

    if ((addr & TARGET_PAGE_MASK)
        != (tlb_addr & (TARGET_PAGE_MASK | TLB_INVALID_MASK))) {
	// Given that a previous load instruction happened, we are sure that
        // that the TLB entry is still in the CPU TLB, if not then the previous
        // instruction caused an error, so we just return with no tlb_fill() call
	return;
    }

    if ((addr & (size - 1)) != 0) {
	// Illegal memory address: not size-aligned 
        return;
    }

    target_ulong phys_address;
    // Getting page physical address
    phys_address = env->tlb_table[mmu_idx][index].paddr;
    // Resolving physical address by adding offset inside the page
    phys_address += addr & ~TARGET_PAGE_MASK;

    uint8_t cache_bits = env->tlb_table[mmu_idx][index].dummy[0];

    int asi;
    // Non-alternate-space Loads (SPARC V9 documention p.73)
    if(env->tl == 0){
       // Trap Level = 0
       if(env->pstate & PS_CLE){
       	       // ASI_PRIMARY_LITTLE
	       asi = 0x88;
       } else {
	       // ASI_PRIMARY 0x80
	       asi = 0x80;
       }
    } else {
       // Trap Level > 0
       if(env->pstate & PS_CLE){
	       // ASI_NUCLEUS_LITTLE 
	       asi = 0x0c;
	       // Note: On some early SPARC V9 implementations,
	       // ASI_PRIMARY_LITTLE may have been used for this
       } else {
	       // ASI_NUCLEUS
	       asi = 0x04;
	       // Note: On some early SPARC V9 implementations,
	       // ASI_PRIMARY may have been used for this
       }	
    }   


    int io = 0;
    if (unlikely(tlb_addr & ~TARGET_PAGE_MASK)) {
	// I/O space
	io = 1;
    }        
    // Otherwise, RAM/ROM , physical memory space, io = 0
    
    // Here, QEMU_Trans_Load is just a dummy argument, not used because
    // is_atomic == 1. So is prefetch_fcn, since type is not prefetch
    flexus_transaction(env, addr, phys_address, pc, QEMU_Trans_Load,
			           size, 1, asi, 0, io, cache_bits);

}

void helper_flexus_rmw_asi(CPUSPARCState *env, target_ulong addr, target_ulong pc, int size, int asi)
{
    // Note: here, I just put the common ASIs between helper_ld_asi and helper_st_asi
    // Since Read-Modify-Write operations are split into a read and a separate write
    // in QEMU's current implementation
    SPARCCPU *cpu = sparc_env_get_cpu(env);
    CPUState *cs = CPU(cpu);

    asi &= 0xff;

    if ((asi < 0x80 && (env->pstate & PS_PRIV) == 0)
        || (cpu_has_hypervisor(env)
            && asi >= 0x30 && asi < 0x80
            && !(env->hpstate & HS_PRIV))) {
    	// Exception: privileged_action
        return;
    }

    if ((addr & (size - 1)) != 0) {
	// Illegal memory address: not aligned 
        return;
    }
    
    // MMU index needed to access the correct
    // entry in the softmmu TLB
    int mmu_idx = 0;

    // Privileged mode: initially false
    //bool priv = false;

    // Note: RMW with non-faulting load ASI is discarded
    // since I'm not sure what needs to be done
    switch (asi) {
    case 0x10: // As if user primary 
    case 0x11: // As if user secondary 
    case 0x18: // As if user primary LE 
    case 0x19: // As if user secondary LE 
    case 0x80: // Primary 
    case 0x81: // Secondary 
    case 0x88: // Primary LE 
    case 0x89: // Secondary LE 
    case 0xe2: // UA2007 Primary block init 
    case 0xe3: // UA2007 Secondary block init
        if ((asi & 0x80) && (env->pstate & PS_PRIV)) {
	  //priv = true;
            if (cpu_hypervisor_mode(env)) {
		// Hypervisor mode
                mmu_idx = 5;
            } else {
                // secondary space access has lowest asi bit equal to 1 
                if (asi & 1) {
		    // Kernel secondary mode
                    mmu_idx = 3;
                } else {
		    // Kernel mode
                    mmu_idx = 2;
                }
            }
        } else {
            // secondary space access has lowest asi bit equal to 1 
            if (asi & 1) {
		// User secondary mode
                mmu_idx = 1;
            } 
		// Else in user mode
		// mmu_idx = 0, initialized when mmu_idx is declared
        }
        break;
    // Note: these are implementation-dependant ASIs
    case 0x14: // Bypass 
    case 0x15: // Bypass, non-cacheable 
    case 0x1c: // Bypass LE 
    case 0x1d: // Bypass, non-cacheable LE 
        {
	    // These ASIs are not translated by
    	    // the MMU; instead, they pass through
	    // their virtual addresses as physical addresses
	    //priv = true;
	
    	    MemoryRegion *mr;
	    hwaddr addr1, l = size;
            mr = address_space_translate(cs->as, addr, &addr1, &l,
                                 true);
	    int is_io = 1;
	    if (memory_region_is_ram(mr)){
		// RAM case
		if(mr->readonly){ 
			// Corrupted Memory Region ?
			return;
		}
		is_io = 0;
	    } else if(memory_region_is_romd(mr)){
		// ROM case
		// Return because we are trying to write 
		// to read-only memory
		return;
	    }
		// Otherwise I/O case, is_io initialized to 1

   	    int cacheBits = 3; // CP, CV set to 1
	    if(asi & 1){
		cacheBits = 0; // if non-cacheable asi set both CP,CV to 0
	    }

            // Cache Physical and Cache Virtual are both set to 1 if cacheable
	    // otherwise cleared to 0 if we have a non-cacheable asi
	    // This is a dummy assignment since there is no TLB entry involved
	    // Since it is a bypass asi
    	    // QEMU_Trans_Load is just a dummy argument, not used because
    	    // is_atomic == 1. So is prefetch_fcn, since type is not prefetch
    	    flexus_transaction(env, addr, addr, pc, QEMU_Trans_Load,
					 size, 1, asi, 0, is_io, cacheBits);
	    return;
        }
    case 0x04: // Nucleus 
    case 0x0c: // Nucleus Little Endian (LE) 
        {
	  //priv = true;
            mmu_idx = 4;
	    break;
        }
    default:
        return;
    }

    int index = (addr >> TARGET_PAGE_BITS) & (CPU_TLB_SIZE - 1);
    target_ulong tlb_addr = env->tlb_table[mmu_idx][index].addr_write;

    if ((addr & TARGET_PAGE_MASK)
        != (tlb_addr & (TARGET_PAGE_MASK | TLB_INVALID_MASK))) {
	// Given that a previous store instruction happened, we are sure that
        // that the TLB entry is still in the CPU TLB, if not then the previous
        // instruction caused an error, so we just return with no tlb_fill() call
	return;
    }

    target_ulong phys_address;
    // Getting page physical address
    phys_address = env->tlb_table[mmu_idx][index].paddr;
    // Resolving physical address by adding offset inside the page
    phys_address += addr & ~TARGET_PAGE_MASK;

    uint8_t cache_bits = env->tlb_table[mmu_idx][index].dummy[0];

    int io = 0;
    if (unlikely(tlb_addr & ~TARGET_PAGE_MASK)) {
	// I/O space
	io = 1;
    }        
    // Otherwise, RAM/ROM , physical memory space, io = 0
    
    // Here, QEMU_Trans_Load is just a dummy argument, not used because
    // is_atomic == 1. So is prefetch_fcn, since type is not prefetch
    flexus_transaction(env, addr, phys_address, pc, QEMU_Trans_Load,
				   size, 1, asi, 0, io, cache_bits);   

}

void helper_flexus_cas_asi(CPUSPARCState *env, target_ulong addr, target_ulong pc, int size, int asi)
{
    // Note: here, I just put the common ASIs between helper_ld_asi and helper_st_asi
    // Since Compare and Swap operations are split into a read and a separate write
    // in QEMU's current implementation (exact same code as RMW, since CAS is essentially RMW)
    SPARCCPU *cpu = sparc_env_get_cpu(env);
    CPUState *cs = CPU(cpu);

    asi &= 0xff;

    if ((asi < 0x80 && (env->pstate & PS_PRIV) == 0)
        || (cpu_has_hypervisor(env)
            && asi >= 0x30 && asi < 0x80
            && !(env->hpstate & HS_PRIV))) {
    	// Exception: privileged_action
        return;
    }

    if ((addr & (size - 1)) != 0) {
	// Illegal memory address: not aligned 
        return;
    }
    
    // MMU index needed to access the correct
    // entry in the softmmu TLB
    int mmu_idx = 0;

    // Privileged mode: initially false
    //bool priv = false;

    // Note: CAS with non-faulting load ASI is discarded
    // since I'm not sure what needs to be done
    switch (asi) {
    case 0x10: // As if user primary 
    case 0x11: // As if user secondary 
    case 0x18: // As if user primary LE 
    case 0x19: // As if user secondary LE 
    case 0x80: // Primary 
    case 0x81: // Secondary 
    case 0x88: // Primary LE 
    case 0x89: // Secondary LE 
    case 0xe2: // UA2007 Primary block init 
    case 0xe3: // UA2007 Secondary block init
        if ((asi & 0x80) && (env->pstate & PS_PRIV)) {
	  //priv = true;
            if (cpu_hypervisor_mode(env)) {
		// Hypervisor mode
                mmu_idx = 5;
            } else {
                // secondary space access has lowest asi bit equal to 1 
                if (asi & 1) {
		    // Kernel secondary mode
                    mmu_idx = 3;
                } else {
		    // Kernel mode
                    mmu_idx = 2;
                }
            }
        } else {
            // secondary space access has lowest asi bit equal to 1 
            if (asi & 1) {
		// User secondary mode
                mmu_idx = 1;
            } 
		// Else in user mode
		// mmu_idx = 0, initialized when mmu_idx is declared
        }
        break;
    // Note: these are implementation-dependant ASIs
    case 0x14: // Bypass 
    case 0x15: // Bypass, non-cacheable 
    case 0x1c: // Bypass LE 
    case 0x1d: // Bypass, non-cacheable LE 
        {
	    // These ASIs are not translated by
    	    // the MMU; instead, they pass through
	    // their virtual addresses as physical addresses
	    //priv = true;
	
    	    MemoryRegion *mr;
	    hwaddr addr1, l = size;
            mr = address_space_translate(cs->as, addr, &addr1, &l,
                                 true);
	    int is_io = 1;
	    if (memory_region_is_ram(mr)){
		// RAM case
		if(mr->readonly){ 
			// Corrupted Memory Region ?
			return;
		}
		is_io = 0;
	    } else if(memory_region_is_romd(mr)){
		// ROM case
		// Return because we are trying to write 
		// to read-only memory
		return;
	    }
		// Otherwise I/O case, is_io initialized to 1

   	    int cacheBits = 3; // CP, CV set to 1
	    if(asi & 1){
		cacheBits = 0; // if non-cacheable asi set both CP,CV to 0
	    }

            // Cache Physical and Cache Virtual are both set to 1 if cacheable
	    // otherwise cleared to 0 if we have a non-cacheable asi
	    // This is a dummy assignment since there is no TLB entry involved
	    // Since it is a bypass asi
    	    // QEMU_Trans_Load is just a dummy argument, not used because
    	    // is_atomic == 1. So is prefetch_fcn, since type is not prefetch
    	    flexus_transaction(env, addr, addr, pc, QEMU_Trans_Load,
					 size, 1, asi, 0, is_io, cacheBits);

	    return;
        }
    case 0x04: // Nucleus 
    case 0x0c: // Nucleus Little Endian (LE) 
        {
	  //priv = true;
            mmu_idx = 4;
	    break;
        }
    default:
        return;
    }

    int index = (addr >> TARGET_PAGE_BITS) & (CPU_TLB_SIZE - 1);
    target_ulong tlb_addr = env->tlb_table[mmu_idx][index].addr_write;

    if ((addr & TARGET_PAGE_MASK)
        != (tlb_addr & (TARGET_PAGE_MASK | TLB_INVALID_MASK))) {
	// Given that a previous store instruction happened, we are sure that
        // that the TLB entry is still in the CPU TLB, if not then the previous
        // instruction caused an error, so we just return with no tlb_fill() call
	return;
    }

    target_ulong phys_address;
    // Getting page physical address
    phys_address = env->tlb_table[mmu_idx][index].paddr;
    // Resolving physical address by adding offset inside the page
    phys_address += addr & ~TARGET_PAGE_MASK;

    uint8_t cache_bits = env->tlb_table[mmu_idx][index].dummy[0];

    int io = 0;
    if (unlikely(tlb_addr & ~TARGET_PAGE_MASK)) {
	// I/O space
	io = 1;
    }        
    // Otherwise, RAM/ROM , physical memory space, io = 0
    
    // Here, QEMU_Trans_Load is just a dummy argument, not used because
    // is_atomic == 1. So is prefetch_fcn, since type is not prefetch
    flexus_transaction(env, addr, phys_address, pc, QEMU_Trans_Load,
				   size, 1, asi, 0, io, cache_bits);    

}

void helper_flexus_prefetch(CPUSPARCState *env, target_ulong addr, target_ulong pc, int mmu_idx, int prefetch_fcn)
{	
	// Taken from Sparc V9 Manual
	// IMPL. DEP. #103(3): The size and alignment in memory of the data block is implementation-
	// dependent; the minimum size is 64 bytes and the minimum alignment is a 64-byte boundary.
	// However, after testing the sparc image, sequential prefetch data addresses
	// were 64 bytes apart.
	
	// Also, since prefetch is ignored in QEMU (no effect), then TLB entries are not available
	// for the given Virtual Addresses, so we have to get physical page address manually. However, this takes
	// way more time than a straight TLB lookup (as used in other helper functions).

	// Furthermore, whether a prefetch instruction has any observable effects is implementation dependant
	// Therefore, it could be treated as a nop
	target_ulong phys_page_addr, phys_address;
	
	// Prefetch Functions 0-4 are implemented, 5-15 are reserved, 16-31 implementation-dependant
	// Unimplemented prefetch functions should be counted as a nop
	// Currently 16-31 are unimplemented		

	int size;
	switch (prefetch_fcn) {
	case 0 ... 3:
		// Prefetch 64 bytes
		if ((addr & (63)) != 0) {
			// Illegal memory address
			// Not page aligned 
        		return;
    		}	
		size = 64;
		break;
	case 4:
		// Prefetch page
		if ((addr & (TARGET_PAGE_SIZE - 1)) != 0) {
			// Illegal memory address
			// Not page aligned 
        		return;
    		}	
		size = TARGET_PAGE_SIZE;
		break;
	default:
		return;
	} 

	CPUState *cs = CPU(sparc_env_get_cpu(env));
	phys_page_addr = cpu_get_phys_page_debug(cs, (addr & TARGET_PAGE_MASK));
	if(phys_page_addr == -1){
		// An error has occurred, address translation failed
		return;
	}
	
	// Adding page offset to page address to get the complete physical address
	phys_address = phys_page_addr + (addr & ~TARGET_PAGE_MASK);

    	MemoryRegion *mr;
	hwaddr addr1, l = 1;
        mr = address_space_translate(cs->as, phys_address, &addr1, &l,
                                 false);

	int asi;
        // Prefetch instructions that do not load from an alternate address space 
        // access the primary address space (ASI_PRIMARY{_LITTLE})
	// (p.207 Sparc V9 Documentation)
	
	if(env->pstate & PS_CLE){
       	       // ASI_PRIMARY_LITTLE
	       asi = 0x88;
        } else {
	       // ASI_PRIMARY
	       asi = 0x80;
        }


    	int io = 0;
 	if(!(memory_region_is_ram(mr) || memory_region_is_romd(mr)))
	{ io = 1; }
    
	flexus_transaction(env, addr, phys_address, pc, QEMU_Trans_Prefetch,
					    size, 0, asi, prefetch_fcn, io, 3);
}

/* Generic Flexus helper functions */
void flexus_transaction(CPUSPARCState *env, target_ulong vaddr, 
		 target_ulong paddr, target_ulong pc, mem_op_type_t type, int size,
		 int atomic, int asi, int prefetch_fcn, int io, uint8_t cache_bits)
{
    SPARCCPU *cpu = sparc_env_get_cpu(env);
    CPUState *cs = CPU(cpu);

    // In Qemu, PhysicalIO address space and PhysicalMemory address
    // space are combined into one (the cpu address space)
    // Operations on this address space may lead to I/O and Physical Memory
    conf_object_t* space = &space_cached;
    space->type = QEMU_AddressSpace;
    space->object = cs->as;
    memory_transaction_t* mem_trans = &mem_trans_cached;
    mem_trans->s.cpu_state = cs;
    mem_trans->s.ini_ptr = space;
    mem_trans->s.pc = pc;
    mem_trans->s.logical_address = vaddr;
    mem_trans->s.physical_address = paddr;
    if(!atomic){
    	mem_trans->s.type = type;
    }
    mem_trans->s.size = size;
    mem_trans->s.atomic = atomic;
    // Cache_Bits: Cache Physical Bit 0
    //		   Cache Virtual Bit 1
    mem_trans->sparc_specific.cache_virtual  = (cache_bits >> 1);
    mem_trans->sparc_specific.cache_physical = (cache_bits & 1);
    // Check to see if CPU in privileged mode (PSTATE.PRIV bit)
    mem_trans->sparc_specific.priv = ((env->pstate & PS_PRIV) != 0);
    mem_trans->sparc_specific.address_space = asi;
    if(type == QEMU_Trans_Prefetch){
    	mem_trans->sparc_specific.prefetch_fcn = prefetch_fcn;
    }
    mem_trans->io = io;
    //to see what is different I am going to print out the mem_trans info

    QEMU_callback_args_t * event_data = &event_data_cached;
    event_data->ncm = &ncm_cached;
    event_data->ncm->space = space;
    event_data->ncm->trans = mem_trans;

    QEMU_execute_callbacks(cpu_proc_num(cs) , QEMU_cpu_mem_trans, event_data);
}

void flexus_insn_fetch_transaction(CPUSPARCState *env, logical_address_t target_vaddr,
		 physical_address_t target_phys_address, logical_address_t pc, mem_op_type_t type,
		 int ins_size, int cond, int annul);

void flexus_insn_fetch_transaction(CPUSPARCState *env, logical_address_t target_vaddr,
		 physical_address_t paddr, logical_address_t pc, mem_op_type_t type,
		 int ins_size, int cond, int annul) {
    SPARCCPU *cpu = sparc_env_get_cpu(env);
    CPUState *cs = CPU(cpu);

    // In Qemu, PhysicalIO address space and PhysicalMemory address
    // space are combined into one (the cpu address space)
    // Operations on this address space may lead to I/O and Physical Memory
    conf_object_t* space = &space_cached;
    space->type = QEMU_AddressSpace;
    space->object = cs->as;
    memory_transaction_t* mem_trans = &mem_trans_cached;
    mem_trans->s.cpu_state = cs;
    mem_trans->s.ini_ptr = space;
    mem_trans->s.pc = pc;
    // the "logical_address" must be the PC for Flexus
    mem_trans->s.logical_address = pc;
    // the "physical_address" is the physical target of the branch.
    mem_trans->s.physical_address = paddr;
    mem_trans->s.type = type;
    mem_trans->s.size = ins_size;
    mem_trans->s.branch_type = cond;
    mem_trans->s.annul = annul;
    mem_trans->sparc_specific.priv = ((env->pstate & PS_PRIV) != 0);
    QEMU_callback_args_t * event_data = &event_data_cached;
    event_data->ncm = &ncm_cached;
    event_data->ncm->space = space;
    event_data->ncm->trans = mem_trans;

    QEMU_execute_callbacks(cpu_proc_num(cs) , QEMU_cpu_mem_trans, event_data);
}

/* Sparc specific helpers */
void helper_flexus_insn_fetch( CPUSPARCState *env,
			       target_ulong pc,
			       target_ulong targ_addr,
			       int ins_size,
			       int cond,
			       int annul ) {
  SPARCCPU *cs = sparc_env_get_cpu(env);
  CPUState *cpu = CPU(cs);
  
  target_ulong vaddr_page = targ_addr & TARGET_PAGE_MASK;
  physical_address_t phys_address = cpu_get_phys_page_debug( cpu, vaddr_page );
  phys_address |= (targ_addr & ~TARGET_PAGE_MASK);

  flexus_insn_fetch_transaction(env, targ_addr, phys_address, pc, QEMU_Trans_Instr_Fetch,
		     ins_size, cond, annul);
}

#endif /* CONFIG_FLEXUS */
