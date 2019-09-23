#include "qemu/osdep.h"
#include "cpu.h"
#include "exec/exec-all.h"
#include "internals.h"
#include "qflex/fa-qflex.h"
#include "qflex/fa-qflex-sim.h"
#include "qflex/fa-qflex-helper.h"
#include "qflex/qflex-log.h"
#include "qflex/json.h"

/* NOTE: Sizes are uint32_t.
 *  size(uint64_t) = size(2 * uint32_t)
 *
 * XREGS: uint64_t
 * PC   : uint64_t
 * CF/VF/NF/ZF : uint32_t
 */
#define ARCH_NUM_REGS           (32)
#define ARCH_XREGS_SIZE         (ARCH_NUM_REGS * sizeof(uint64_t))

#define ARCH_XREGS_OFFST        (0)                             // 0
#define ARCH_PC_OFFST           (32 * 2)                        // 64
#define ARCH_PSTATE_FLAG_OFFST  (ARCH_PC_OFFST + 2)             // 66
#define ARCH_PSTATE_NF_MASK     (0)    // 64 bit 0
#define ARCH_PSTATE_ZF_MASK     (1)    // 64 bit 1
#define ARCH_PSTATE_CF_MASK     (2)    // 64 bit 2
#define ARCH_PSTATE_VF_MASK     (3)    // 64 bit 3

/* (Un)serializes memory, for FPGA use.
 */
void* fa_qflex_pack_archstate(CPUState *cpu) {
    CPUARMState *env = cpu->env_ptr;
    uint32_t *buffer = calloc(FA_QFLEX_ARCH_STATE_SIZE, sizeof(uint32_t));
    if (buffer == NULL) {
        qflex_log_mask(QFLEX_LOG_GENERAL,
                       "Couldn't allocate memory for packing architectural state\n");
    }

    memcpy(&buffer[ARCH_XREGS_OFFST],     &env->xregs, ARCH_XREGS_SIZE);
    memcpy(&buffer[ARCH_PC_OFFST],        &env->pc, sizeof(uint64_t));
    uint32_t nzcv =
            ((env->CF) ? 1 << ARCH_PSTATE_CF_MASK : 0) |
            ((env->VF & (1<<31)) ? 1 << ARCH_PSTATE_VF_MASK : 0) |
            ((env->NF & (1<<31)) ? 1 << ARCH_PSTATE_NF_MASK : 0) |
            (!(env->ZF) ? 1 << ARCH_PSTATE_ZF_MASK : 0);
    memcpy(&buffer[ARCH_PSTATE_FLAG_OFFST], &nzcv, sizeof(uint32_t));

    return buffer;
}

void fa_qflex_unpack_archstate(CPUState *cpu, uint32_t *buffer) {
    CPUARMState *env = cpu->env_ptr;
    memcpy(&env->xregs, &buffer[0], ARCH_XREGS_SIZE);
    env->pc = ((uint64_t) buffer[ARCH_PC_OFFST + 1] << 32) | buffer[ARCH_PC_OFFST];

    uint32_t nzcv = buffer[ARCH_PSTATE_FLAG_OFFST];
    env->CF = (nzcv & ARCH_PSTATE_CF_MASK) ? 1 : 0;
    env->VF = (nzcv & ARCH_PSTATE_VF_MASK) ? (1 << 31) : 0;
    env->NF = (nzcv & ARCH_PSTATE_NF_MASK) ? (1 << 31) : 0;
    env->ZF = !(nzcv & ARCH_PSTATE_ZF_MASK) ? 1 : 0;
}


/* Start JSON CPUArchState utils
 * NOTE: JSON example ( They should actually be in HEXADECIMAL) :
 * {"xregs":[0000000000001f9f,000000000000123c,0000000000000000,00000000202defb0,
 * 00000000202dd010,0000000000000886,0000ffffaac640d0,00000000202fffff,00000000202ff000,
 * 0000ffffaac649f8,0000000000000000,0000000000000018,00000000000003f3,0000000000000000,
 * 00000000000003f3,0000ffffaac96000,0000ffffaac7b7d8,0000ffffaac64018,0000000000000a03,
 * 0000000000400658,0000000000000000,0000000000000000,0000000000000000,0000000000000000,
 * 0000000000000000,0000000000000000,0000000000000000,0000000000000000,0000000000000000,
 * 0000ffffc8909d40,00000000004005d8,0000ffffc8909d40],
 * "pc":00000000004005e4,"nzcv":00000004}
 */
/**
 * @brief fa_qflex_cpu2json
 * @param cpu    CPUState to update
 * @param size   Returned size of the JSON buffer
 */
char* fa_qflex_cpu2json(CPUState *cpu, size_t* size) {
    CPUARMState *env = cpu->env_ptr;
    char *json;
    json_value_s root;
    json_object_s objects;
    objects.length = 0;
    root.type = json_type_object;
    root.payload = &objects;
    json_object_element_s* head;

    //* XREGS packing {
    json_array_element_s xregs_aobjs[ARCH_NUM_REGS];
    json_value_s xregs_vals[ARCH_NUM_REGS];
    json_string_s xregs[ARCH_NUM_REGS];
    char xregs_nums[ARCH_NUM_REGS][ULONG_HEX_MAX];
    for(int i = 0; i < ARCH_NUM_REGS; i++) {
        snprintf(xregs_nums[i], ULONG_HEX_MAX+1, "%0"XSTR(ULONG_HEX_MAX)"lx", env->xregs[i]);
        xregs[i].string = xregs_nums[i];
        xregs[i].string_size = ULONG_HEX_MAX;
        xregs_vals[i].payload = &xregs[i];
        xregs_vals[i].type = json_type_string;
        xregs_aobjs[i].value = &xregs_vals[i];
        json_array_element_s* next = (i == ARCH_NUM_REGS-1) ? NULL : &xregs_aobjs[i+1];
        xregs_aobjs[i].next = next;
    }

    json_array_s xregs_array = {.start = &xregs_aobjs[0], .length = ARCH_NUM_REGS };
    json_value_s xregs_val = {.payload = &xregs_array, .type = json_type_array};
    json_string_s xregs_name = {.string = "xregs", .string_size = strlen("xregs")};
    json_object_element_s xregs_obj = {.value = &xregs_val, .name = &xregs_name, .next = NULL};
    objects.start = &xregs_obj;
    head = &xregs_obj;
    objects.length++;
    // */ }

    //* PC packing {
    json_string_s pc;

    char pc_num[ULONG_HEX_MAX + 1];
    snprintf(pc_num, ULONG_HEX_MAX+1,"%0"XSTR(ULONG_HEX_MAX)"lx", env->pc);
    pc.string = pc_num;
    pc.string_size = ULONG_HEX_MAX;

    json_value_s pc_val = {.payload = &pc, .type = json_type_string};
    json_string_s pc_name = {.string = "pc", .string_size = strlen("pc")};
    json_object_element_s pc_obj = {.value = &pc_val, .name = &pc_name, .next = NULL};
    head->next = &pc_obj;
    head = &pc_obj;
    objects.length++;
    // */ }

    //* NZCV flags packing {
    json_string_s nzcv;

    char nzcv_num[UINT_HEX_MAX + 1];
    uint32_t nzcv_flags =
            ((env->CF) ? 1 << ARCH_PSTATE_CF_MASK : 0) |
            ((env->VF & (1<<31)) ? 1 << ARCH_PSTATE_VF_MASK : 0) |
            ((env->NF & (1<<31)) ? 1 << ARCH_PSTATE_NF_MASK : 0) |
            (!(env->ZF) ? 1 << ARCH_PSTATE_ZF_MASK : 0);
    snprintf(nzcv_num, UINT_HEX_MAX+1, "%0"XSTR(UINT_HEX_MAX)"x", nzcv_flags);
    nzcv.string = nzcv_num;
    nzcv.string_size = UINT_HEX_MAX;

    json_value_s nzcv_val = {.payload = &nzcv, .type = json_type_string};
    json_string_s nzcv_name = {.string = "nzcv", .string_size = strlen("nzcv")};
    json_object_element_s nzcv_obj = {.value = &nzcv_val, .name = &nzcv_name, .next = NULL};
    head->next = &nzcv_obj;
    head = &nzcv_obj;
    objects.length++;
    // */ }

    json = json_write_minified(&root, size);
    return json;
}

/**
 * @brief fa_qflex_json2cpu
 * @param cpu    CPUState to update
 * @param buffer Readed JSON buffer
 * @param size   Size of the JSON buffer
 */
void fa_qflex_json2cpu(CPUState *cpu, char* buffer, size_t size) {
    CPUARMState *env = cpu->env_ptr;
    json_value_s* root = json_parse(buffer, size);
    json_object_s* objects = root->payload;
    json_object_element_s* curr;
    curr = objects->start;
    do {
        if(!strcmp(curr->name->string, "xregs")) {
            assert(curr->value->type == json_type_array);
            json_array_s* xregs = curr->value->payload;
            json_array_element_s* curr_reg = xregs->start;
            json_string_s* reg_val;
            for(int i = 0; i < ARCH_NUM_REGS; i++) {
                assert(curr_reg->value->type == json_type_string);
                reg_val = curr_reg->value->payload;
                env->xregs[i] = strtol(reg_val->string, NULL, 16);
                curr_reg = curr_reg->next;
            }
        } else if(!strcmp(curr->name->string, "pc")) {
            assert(curr->value->type == json_type_string);
            json_string_s* pc = curr->value->payload;
            env->pc = strtol(pc->string, NULL, 16);
        } else if(!strcmp(curr->name->string, "nzcv")) {
                assert(curr->value->type == json_type_string);
            json_string_s* json_nzcv = curr->value->payload;
            uint32_t nzcv = strtol(json_nzcv->string, NULL, 16);
            env->CF = (nzcv & ARCH_PSTATE_CF_MASK) ? 1 : 0;
            env->VF = (nzcv & ARCH_PSTATE_VF_MASK) ? (1 << 31) : 0;
            env->NF = (nzcv & ARCH_PSTATE_NF_MASK) ? (1 << 31) : 0;
            env->ZF = !(nzcv & ARCH_PSTATE_ZF_MASK) ? 1 : 0;
        }
        curr = curr->next;
    } while(curr);
    free(root);
}
/* End JSON CPUArchState utils */


static inline bool fa_qflex_mmu_guest_logical_to_physical(CPUState *cs, target_ulong addr, MMUAccessType access_type,
                                            MemTxAttrs *attrs, hwaddr *phys_addr, target_ulong *page_size) {

    ARMCPU *cpu = ARM_CPU(cs);
    CPUARMState *env = &cpu->env;
    int prot;
    bool ret;
    uint32_t fsr;
    ARMMMUFaultInfo fi = {};
    ARMMMUIdx mmu_idx = core_to_arm_mmu_idx(env, cpu_mmu_index(env, false));

    ret = QFLEX_GET_F(get_phys_addr)(env, addr, 0, mmu_idx,
                                     phys_addr, attrs, &prot, page_size, &fsr, &fi);

    if(unlikely(ret)) {
        return ret; //fa_qflex_deliver_fault(cs, addr, access_type, fsr, &fi);
    }

    return 0;
}

/* Inspired from tlb_set_page_with_attrs() (cputlb.c)
 */
static inline void fa_qflex_guest_phys_page_to_host_phys_page(CPUState *cpu, hwaddr guest_phys_page, target_ulong page_size, MemTxAttrs attrs,
                                                hwaddr *host_phys_page) {
    MemoryRegionSection *section;
    hwaddr xlat, sz;
    sz = page_size;
    int asidx = cpu_asidx_from_attrs(cpu, attrs);
    section = address_space_translate_for_iotlb(cpu, asidx, guest_phys_page, &xlat, &sz);
    *host_phys_page = (uintptr_t)memory_region_get_ram_ptr(section->mr) + xlat;
}

bool fa_qflex_get_page(CPUState *cpu, target_ulong addr, MMUAccessType access_type,  uint64_t *host_phys_page, uint64_t *page_size) {
    int ret;
    hwaddr paddr = 0, guest_phys_page = 0;
    MemTxAttrs attrs = {};

    ret = fa_qflex_mmu_guest_logical_to_physical(cpu, addr, access_type,
                                                 &attrs, &paddr, page_size);
    // FAULT was generated
    if(unlikely(ret)) { return ret; }

    guest_phys_page = paddr & TARGET_PAGE_MASK;

    rcu_read_lock();
    fa_qflex_guest_phys_page_to_host_phys_page(cpu, guest_phys_page, *page_size, attrs, host_phys_page);
    rcu_read_unlock();

    return ret;
}

/* Empty GETTERs in case CONFIG_FA_QFLEX is disabled.
 * To see real functions, see original file (op_helper/helper/helper-a64.c)
 */
#ifndef CONFIG_FA_QFLEX
/* op_helper.c
 */
void QFLEX_GET_F(deliver_fault)(ARMCPU *cpu, vaddr addr, MMUAccessType access_type,
                                uint32_t fsr, uint32_t fsc, ARMMMUFaultInfo *fi) { return; }

/* helper.c
 */
bool QFLEX_GET_F(get_phys_addr)(CPUARMState *env, target_ulong address,
                                MMUAccessType access_type, ARMMMUIdx mmu_idx,
                                hwaddr *phys_ptr, MemTxAttrs *attrs, int *prot,
                                target_ulong *page_size, uint32_t *fsr,
                                ARMMMUFaultInfo *fi) { return false; }
#endif

