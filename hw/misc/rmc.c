/*
 * QEMU RMC PCI device
 *
 * Copyright (c) 2015 EPFL
 * 
 * Authors:
 * Damien Hilloulin 
 * Jan Alexander Wessel <Jan.Wessel@tum.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "hw/pci/pci.h"
#include "qemu/timer.h"
#include "qemu/main-loop.h" /* iothread mutex */
#include "qapi/visitor.h"
#include "hw/misc/RMC.h"
#include "net/net.h"
#include "arpa/inet.h"

// Msutherl: QFlex Includes/headers
#include "target/arm/cpu.h"
#include "target/arm/cpu-qom.h"
#include "qemu/thread.h" // if running in deterministic PTH, need macros

#define PCI_BAR 0
#define RMC(obj)        OBJECT_CHECK(RMCState, obj, "rmc")

#define FACT_IRQ        0x00000001
#define DMA_IRQ         0x00000100

#define DMA_START       0x40000
#define DMA_SIZE        4096

#ifdef RMC_DEBUG
#define DRMC_Print(M, ...) fprintf(stdout, "QRMC_DEBUG %s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define DRMC_Print(M, ...)
#endif // #ifdef RMC_DEBUG

#define RMC_QP_BITS 8
#define RMC_QP_BITMASK ( (1<<RMC_QP_BITS)-1 )
#define RMC_QPID_SHIFT (32-RMC_QP_BITS) // 32 since mmios are 4B
#define RMC_QPID_SHIFT_64 (64-RMC_QP_BITS) // for 8B datatypes
#define RMC_MAX_NUM_QPS (1<<RMC_QP_BITS)
#define RMC_QP_INV_BITMASK ~((uint64_t)RMC_QP_BITMASK << RMC_QPID_SHIFT_64)

/* Msutherl: Replaced the previous structure with scalable RMC QP tracking.
 * - wq_arr and cq_arr store the actual QPs as defined in the HW/SW interface
 * - wq_arr_gvas and cq_arr_gvas store the gvaddresses of both, before the QEMU
 *   interface is used to translate them to hVas in wq/cq_init
 * - wq_arr_translated is a bunch of booleans to show which are initialized yet
 */
typedef struct {
    rmc_cq_t cq_arr[RMC_MAX_NUM_QPS];
    rmc_wq_t wq_arr[RMC_MAX_NUM_QPS];

    uint64_t cq_arr_gvas[RMC_MAX_NUM_QPS];
    uint64_t wq_arr_gvas[RMC_MAX_NUM_QPS];
    bool wq_arr_used[RMC_MAX_NUM_QPS];
    bool cq_arr_used[RMC_MAX_NUM_QPS];

    hwaddr wq_arr_gpas[RMC_MAX_NUM_QPS];
    hwaddr cq_arr_gpas[RMC_MAX_NUM_QPS];
    bool wq_arr_translated[RMC_MAX_NUM_QPS];
    bool cq_arr_translated[RMC_MAX_NUM_QPS];
} rmc_qp_storage;

typedef struct {
    hwaddr lbuf_array[RMC_MAX_NUM_QPS];
} rmc_lbuf_storage;

rmc_qp_storage my_qps;
rmc_lbuf_storage my_lbufs;

hwaddr paddr = 0;
int RMC_existence = 0;
int RMC_initialised = 0;
int local_tid = 0;
long long int ITT[MAX_NUM_WQ][4];
char *prevaddr, *curraddr;
uint64_t cr3value;
uint8_t cq_head, cq_SR, wq_tail, wq_SR; // TODO: these need to be qp-private
//rmc_cq_t cq;
//rmc_wq_t wq;
void *RMC_NICState;
MACAddr RMC_macaddr;
MACAddr MAC_basis = {{0xff,0xff,0xff,0xff,0xff,0xff}};
hwaddr local_shared_space;
uint64_t    gVA_ctx;
hwaddr      gPA_ctx;
uint8_t RMC_protocol_definition[] = {0x08, 0x00};
uint8_t RMC_ip_protocol_definition = 200;
CPUState *RMC_private_cpu;
uint16_t local_nid;
uint8_t local_cid;

buffer_t RCP_buffer, RRPP_buffer;
RGP_state_t RGP_current_state;
int RGP_length_request, RGP_offset_request, RGP_op, RGP_dest_nid, RGP_local_tid;
uint64_t RGP_buf_addr, RGP_offset;
uint8_t RGP_cid;
hwaddr RGP_phys_wq_addr;
void *RGP_wq_temp_buffer;
read_request_eth_frame_t RGP_read_packet;
write_request_eth_frame_t RGP_write_packet;
uint64_t RRPP_current_offset;
hwaddr RRPP_current_guest_phys;
write_request_eth_frame_t *RRPP_write_frame;
read_request_eth_frame_t *RRPP_read_frame;
read_completion_eth_frame_t RRPP_read_reply;
write_completion_eth_frame_t RRPP_write_reply;
RRPP_state_t RRPP_current_state;
void *RRPP_data;
uint8_t RRPP_op;
uint64_t RCP_current_offset;
uint8_t RCP_current_tid;
write_completion_eth_frame_t *RCP_frame_write;
read_completion_eth_frame_t *RCP_frame_read;
vaddr RCP_current_guest_virt;
hwaddr RCP_current_guest_phys;
RCP_state_t RCP_current_state;

/* Following are functions for correctly setting up the RMC as a PCI device */
typedef struct {
    PCIDevice pdev;
    MemoryRegion mmio;

    /* QFlex/AARCH64 required state:
     * - ttbrs (replaces CR3)
     */
    uint64_t proc_ttbr0;
    uint64_t proc_ttbr1;

    //uint64_t cr3;
    uint64_t logical_address;
    uint32_t irq_status;

#define RMC_DMA_RUN             0x1
#define RMC_DMA_DIR(cmd)        (((cmd) & 0x2) >> 1)
# define RMC_DMA_FROM_PCI       0
# define RMC_DMA_TO_PCI         1
#define RMC_DMA_IRQ             0x4
    struct dma_state {
        dma_addr_t src;
        dma_addr_t dst;
        dma_addr_t cnt;
        dma_addr_t cmd;
    } dma;
    QEMUTimer dma_timer;
    char dma_buf[DMA_SIZE];
    uint64_t dma_mask;
    uint16_t nid;
    uint8_t cid;

    uint8_t last_qpid;
    bool qpid_used;
} RMCState;

static void rmc_raise_irq(RMCState *rmc, uint32_t val)
{
    rmc->irq_status |= val;
    if (rmc->irq_status) {
        pci_set_irq(&rmc->pdev, 1);
    }
}

static void rmc_lower_irq(RMCState *rmc, uint32_t val)
{
    rmc->irq_status &= ~val;

    if (!rmc->irq_status) {
        pci_set_irq(&rmc->pdev, 0);
    }
}

static bool within(uint32_t addr, uint32_t start, uint32_t end)
{
    return start <= addr && addr < end;
}

static void rmc_check_range(uint32_t addr, uint32_t size1, uint32_t start,
                uint32_t size2)
{
    uint32_t end1 = addr + size1;
    uint32_t end2 = start + size2;

    if (within(addr, start, end2) &&
            end1 > addr && within(end1, start, end2)) {
        return;
    }

    hw_error("RMC: DMA range 0x%.8x-0x%.8x out of bounds (0x%.8x-0x%.8x)!",
            addr, end1 - 1, start, end2 - 1);
}

static dma_addr_t rmc_clamp_addr(const RMCState *rmc, dma_addr_t addr)
{
    dma_addr_t res = addr & rmc->dma_mask;

    if (addr != res) {
        printf("RMC: clamping DMA %#.16"PRIx64" to %#.16"PRIx64"!\n", addr, res);
    }

    return res;
}

static void rmc_dma_timer(void *opaque)
{
    RMCState *rmc = opaque;
    bool raise_irq = false;

    if (!(rmc->dma.cmd & RMC_DMA_RUN)) {
        return;
    }

    if (RMC_DMA_DIR(rmc->dma.cmd) == RMC_DMA_FROM_PCI) {
        uint32_t dst = rmc->dma.dst;
        rmc_check_range(dst, rmc->dma.cnt, DMA_START, DMA_SIZE);
        dst -= DMA_START;
        pci_dma_read(&rmc->pdev, rmc_clamp_addr(rmc, rmc->dma.src),
                rmc->dma_buf + dst, rmc->dma.cnt);
    } else {
        uint32_t src = rmc->dma.src;
        rmc_check_range(src, rmc->dma.cnt, DMA_START, DMA_SIZE);
        src -= DMA_START;
        pci_dma_write(&rmc->pdev, rmc_clamp_addr(rmc, rmc->dma.dst),
                rmc->dma_buf + src, rmc->dma.cnt);
    }

    rmc->dma.cmd &= ~RMC_DMA_RUN;
    if (rmc->dma.cmd & RMC_DMA_IRQ) {
        raise_irq = true;
    }

    if (raise_irq) {
        rmc_raise_irq(rmc, DMA_IRQ);
    }
}

static void dma_rw(RMCState *rmc, bool write, dma_addr_t *val, dma_addr_t *dma,
                bool timer)
{
    if (write && (rmc->dma.cmd & RMC_DMA_RUN)) {
        return;
    }

    if (write) {
        *dma = *val;
    } else {
        *val = *dma;
    }

    if (timer) {
        timer_mod(&rmc->dma_timer, qemu_clock_get_ms(QEMU_CLOCK_VIRTUAL) + 100);
    }
}

static uint64_t rmc_mmio_read(void *opaque, hwaddr addr, unsigned size)
{
    RMCState *rmc = opaque;
    uint64_t val = ~0ULL;

    if (size != 4) {
        return val;
    }

    switch (addr) {
    case 0x00:
        val = 0x010000EDU;
        break;
    case 0x04:
        val = 0;
        break;
    case 0x08:
        break;
    case 0x20:
        val = 0;
        break;
    case 0x24:
        val = rmc->irq_status;
        break;
    case 0x80:
        dma_rw(rmc, false, &val, &rmc->dma.src, false);
        break;
    case 0x88:
        dma_rw(rmc, false, &val, &rmc->dma.dst, false);
        break;
    case 0x90:
        dma_rw(rmc, false, &val, &rmc->dma.cnt, false);
        break;
    case 0x98:
        dma_rw(rmc, false, &val, &rmc->dma.cmd, false);
        break;
    }

    return val;
}

static inline uint8_t decode_qpaddr_hibits(uint64_t val) {
    uint8_t id = (val >> RMC_QPID_SHIFT) & RMC_QP_BITMASK;
    return id;
}

static void write_qp_vaddr_hibits(bool is_wq, uint8_t qp_id, uint64_t val) {
    if( is_wq ) { 
        assert( my_qps.wq_arr_used[qp_id] == false );
        my_qps.wq_arr_gvas[qp_id] = ((val & 0xFFFFFFFF) << 32);
        my_qps.wq_arr_gvas[qp_id] &= RMC_QP_INV_BITMASK;
        my_qps.wq_arr_used[qp_id] = true;
    } else {
        assert( my_qps.cq_arr_used[qp_id] == false );
        my_qps.cq_arr_gvas[qp_id] = ((val & 0xFFFFFFFF) << 32);
        my_qps.cq_arr_gvas[qp_id] &= RMC_QP_INV_BITMASK;
        my_qps.cq_arr_used[qp_id] = true;
    }
}

static uint64_t write_qp_vaddr_lobits(bool is_wq, uint8_t qp_id, uint64_t val) {
    if( is_wq ) { 
        assert( my_qps.wq_arr_used[qp_id] == true);
        my_qps.wq_arr_gvas[qp_id] |= (val & 0xFFFFFFFF);
        return my_qps.wq_arr_gvas[qp_id];
    } else {
        assert( my_qps.cq_arr_used[qp_id] == true);
        my_qps.cq_arr_gvas[qp_id] |= (val & 0xFFFFFFFF);
        return my_qps.cq_arr_gvas[qp_id];
    }
}

static inline void write_hibits(uint64_t* dst, uint64_t val) {
    *dst |= ((val & 0xFFFFFFFF) << 32);
}

static inline void write_lobits(uint64_t* dst, uint64_t val) {
    *dst = (val & 0xFFFFFFFF);
}

static void rmc_mmio_write(void *opaque, hwaddr addr, uint64_t val,
                unsigned size)
{
    RMCState *rmc = opaque;
    uint64_t finaladdr;
    uint8_t qpid;
    int i;
    hwaddr xlat;

    switch(addr) {
        case 0x04: 
            DRMC_Print("Written into 0x04 (LO ttbr0): %016lX\n", val);
            write_lobits(&(rmc->proc_ttbr0),val);
            return ;
        case 0x08: 
            DRMC_Print("Written into 0x08 (HI ttbr0): %016lX\n", val);
            write_hibits(&(rmc->proc_ttbr0),val);
            DRMC_Print("RMC TTBR0 %#018lX\n", rmc->proc_ttbr0);
            return ;
        case 0x0C: /* 4 bytes */
            DRMC_Print("Written into 0x0C (LO ttbr1): %#010lx\n", val);
            write_lobits(&(rmc->proc_ttbr1),val);
            return ;
        case 0x10: /* 4 bytes */
            DRMC_Print("Written into 0x10 (HI ttbr1): %#010lx\n", val);
            write_hibits(&(rmc->proc_ttbr1),val);
            DRMC_Print("RMC TTBR1 %#018lX\n", rmc->proc_ttbr1);
            return ;
        case 0x14: /* 4 bytes, WQ low virt addr */
            DRMC_Print("Written into 0x14 (WQ low): %#010lx\n", val);
            assert( rmc->qpid_used );
            finaladdr = write_qp_vaddr_lobits(true,rmc->last_qpid,val);
            DRMC_Print("WQ ID [%d] VAddr: %#010lx\n", rmc->last_qpid, finaladdr);
            rmc->last_qpid = 0;
            rmc->qpid_used = false;
            return ;
        case 0x18: /* 4B, WQ high virt addr. Bits 63-56 are for the WQ id,
                      since those bits are unused by the Linux kernel user mm.
                      see: https://www.kernel.org/doc/Documentation/x86/x86_64/mm.txt
                      and: https://www.kernel.org/doc/Documentation/arm64/memory.txt
                    */
            DRMC_Print("Written into 0x18 (WQ high): %#010lx\n", val);
            qpid = decode_qpaddr_hibits(val);
            write_qp_vaddr_hibits(true,qpid,val);
            rmc->last_qpid = qpid;
            rmc->qpid_used = true;
            return ;
        case 0x1c: /* 4B, CQ low virt addr */
            DRMC_Print("Written into 0x1c (CQ low): %#010lx\n", val);
            assert( rmc->qpid_used );
            finaladdr = write_qp_vaddr_lobits(false,rmc->last_qpid,val);
            DRMC_Print("CQ ID [%d] VAddr: %#010lx\n", rmc->last_qpid, finaladdr);
            rmc->last_qpid = 0;
            rmc->qpid_used = false;
            return ;
        case 0x20: /* 4B: CQ high virt addr, bits 63-55 as above */
            DRMC_Print("Written into 0x20 (CQ high): %#010lx\n", val);
            qpid = decode_qpaddr_hibits(val);
            write_qp_vaddr_hibits(false,qpid,val);
            rmc->last_qpid = qpid;
            rmc->qpid_used = true;
            return ;
        case 0x24: /* 4B: Context low virt addr */
            DRMC_Print("Written into 0x24 (Ctx LO): %#010lx\n", val);
            gVA_ctx = val;
            return ;
        case 0x28: /* 4B: Context hi virt addr */
            DRMC_Print("Written into 0x28 (Ctx HI): %#010lx\n", val);
            gVA_ctx |= (val << 32);
            DRMC_Print("CTX Base address (gVA) %#018lX\n", gVA_ctx);
            return ;
        case 0xF4: /* 4 bytes */
            printf("\nRMC is now configured and reachable under the destination nid %u and with the cid %u\n", local_nid, local_cid);

            PTH_UPDATE_CONTEXT;

            RMC_buffer_init(&RCP_buffer, 1000);
            RMC_buffer_init(&RRPP_buffer, 1000);
            RCP_current_state = RCP_Decode;
            RRPP_current_state = RRPP_Decode;
            RGP_current_state = RGP_Poll;

            /* set registers so that the translation can be done.
             * - save MMU translation state to tmp vars (e.g., if another 
             *   process is executing in the cpu loop, dont corrupt its
             *   architectural state)
             * - translate all of the control path according to the process
             *   that passed the addresses
             * - restore the state */
            ARMCPU* cpu = ARM_CPU(PTH(current_cpu));
            CPUArchState* arm_cpu_state = &(cpu->env);

            uint8_t current_el = arm_current_el(arm_cpu_state) & 0x3;
            target_ulong saved_ttbr0 = arm_cpu_state->cp15.ttbr0_el[current_el];
            //target_ulong saved_ttbr1 = arm_cpu_state->cp15.ttbr1_el[current_el];

            arm_cpu_state->cp15.ttbr0_el[current_el] = rmc->proc_ttbr0;
            //arm_cpu_state->cp15.ttbr1_el[current_el] = rmc->proc_ttbr1;

            /* 
             * - Traverse all of rmc qp/lbuf/etc. and do translations
             *   to fill all gpaddr structures.
             * - Lbuf array is right now unused, because I can just use
             *   the hVaddr in the WQE and translate it on the fly.
             *   (remove it in future)
             */

            for( i = 0; i < RMC_MAX_NUM_QPS; ++i ) { 
                if( my_qps.wq_arr_used[i] ) {
                    xlat = cpu_get_phys_page_debug( PTH(current_cpu), my_qps.wq_arr_gvas[i] & TARGET_PAGE_MASK );
                    my_qps.wq_arr_gpas[i] = xlat | (my_qps.wq_arr_gvas[i] & ~TARGET_PAGE_MASK);
                    my_qps.wq_arr_translated[i] = true;
                }
                if( my_qps.cq_arr_used[i] ) {
                    xlat = cpu_get_phys_page_debug( PTH(current_cpu), my_qps.cq_arr_gvas[i] & TARGET_PAGE_MASK );
                    my_qps.cq_arr_gpas[i] = xlat | (my_qps.cq_arr_gvas[i] & ~TARGET_PAGE_MASK);
                    my_qps.cq_arr_translated[i] = true;
                }
            }

            xlat = cpu_get_phys_page_debug( PTH(current_cpu), gVA_ctx & TARGET_PAGE_MASK );
            gPA_ctx = xlat | (gVA_ctx & ~TARGET_PAGE_MASK);

            /* restore processor state */
            arm_cpu_state->cp15.ttbr0_el[current_el] = saved_ttbr0;
            //arm_cpu_state->cp15.ttbr1_el[current_el] = saved_ttbr1;

            RMC_initialised = 0;
            local_tid = 0;
            return;
    }

    if (addr < 0x80 && size != 4) {
        return;
    }

    if (addr >= 0x80 && size != 4 && size != 8) {
        return;
    }

    switch (addr) {
        case 0x04:
            break;
        case 0x08:
            break;
        case 0x20:
            break;
        case 0x60:
            rmc_raise_irq(rmc, val);
            break;
        case 0x64:
            rmc_lower_irq(rmc, val);
            break;
        case 0x80:
            dma_rw(rmc, true, &val, &rmc->dma.src, false);
            break;
        case 0x88:
            dma_rw(rmc, true, &val, &rmc->dma.dst, false);
            break;
        case 0x90:
            dma_rw(rmc, true, &val, &rmc->dma.cnt, false);
            break;
        case 0x98:
            if (!(val & RMC_DMA_RUN)) {
                break;
            }
            dma_rw(rmc, true, &val, &rmc->dma.cmd, true);
            break;
    }
}

static const MemoryRegionOps rmc_mmio_ops = {
    .read = rmc_mmio_read,
    .write = rmc_mmio_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static int pci_rmc_init(PCIDevice *pdev)
{
    RMCState *rmc = DO_UPCAST(RMCState, pdev, pdev);
    uint8_t *pci_conf = pdev->config;
    
    local_nid = rmc->nid;
    local_cid = rmc->cid;

    timer_init_ms(&rmc->dma_timer, QEMU_CLOCK_VIRTUAL, rmc_dma_timer, rmc);

    pci_config_set_interrupt_pin(pci_conf, 1);

    memory_region_init_io(&rmc->mmio, OBJECT(rmc), &rmc_mmio_ops, rmc,
                    "rmc-mmio", 1 << 20);
    pci_register_bar(pdev, PCI_BAR, PCI_BASE_ADDRESS_SPACE_MEMORY, &rmc->mmio);

    return 0;
}

static void pci_rmc_uninit(PCIDevice *pdev)
{
    RMCState *rmc = DO_UPCAST(RMCState, pdev, pdev);

    timer_del(&rmc->dma_timer);
}

static void rmc_obj_uint64(Object *obj, struct Visitor *v, const char* name, void *opaque, Error **errp)
{
    uint64_t* val = opaque;
    visit_type_uint64(v, name, val, errp);
}

static void rmc_instance_init(Object *obj)
{
    RMCState *rmc = RMC(obj);
    rmc->dma_mask = (1UL << 28) - 1;
    object_property_add(obj, "dma_mask", "uint64", rmc_obj_uint64,
                    rmc_obj_uint64, NULL, &rmc->dma_mask, NULL);
    object_property_add(obj, "nid", "uint16", rmc_obj_uint64,
                    rmc_obj_uint64, NULL, &rmc->nid, NULL);
    object_property_add(obj, "cid", "uint8", rmc_obj_uint64,
                    rmc_obj_uint64, NULL, &rmc->cid, NULL);
}

static void rmc_class_init(ObjectClass *class, void *data)
{
    PCIDeviceClass *k = PCI_DEVICE_CLASS(class);

    k->init = pci_rmc_init;
    k->exit = pci_rmc_uninit;
    k->vendor_id = PCI_VENDOR_ID_QEMU;
    k->device_id = 0x10f0;// temporary id
    k->revision = 0x10;
    k->class_id = PCI_CLASS_OTHERS;
    RMC_existence = 1;
}

static void pci_rmc_register_types(void)
{
    static const TypeInfo rmc_info = {
        .name          = "rmc",
        .parent        = TYPE_PCI_DEVICE,
        .instance_size = sizeof(RMCState),
        .instance_init = rmc_instance_init,
        .class_init    = rmc_class_init,
    };

    type_register_static(&rmc_info);
}
type_init(pci_rmc_register_types)

/* Returns the physical address of the work queue */
hwaddr RMC_get_phys_wq(size_t wq_id) {
    //assert( my_qps.wq_arr_translated[wq_id] == true );
	return my_qps.wq_arr_gpas[wq_id];
}

/* Returns the physical address of the work queue */
hwaddr RMC_get_phys_cq(size_t cq_id) {
    //assert( my_qps.cq_arr_translated[cq_id] == true );
	return my_qps.cq_arr_gpas[cq_id];
}

/* Returns the physical address of the ctx ptr*/
hwaddr RMC_get_phys_ctx(void) {
	return gPA_ctx;
}

/* Whether RMC option has been passed to qemu during startup */
int RMC_does_exist(void) {
	return RMC_existence;
}

/* Takes guest virtual address, changes processor state to correct page
 * table, translates to guest physical and restores previous processor
 * state. Returns guest physical address */
hwaddr RMC_translate(vaddr guest_virt) {
	target_ulong vaddr_page = guest_virt & TARGET_PAGE_MASK;

    /* set registers so that the translation can be done */
    /* TODO: Use ARM64 state here after I get the control path running */
    /*
    X86CPU *cpu = X86_CPU(RMC_private_cpu);
    CPUX86State *env = &cpu->env;
    target_ulong temp_cr3 = env->cr[3];
    target_ulong temp_hflags = env->hflags;
    env->cr[3] = cr3value;
    env->hflags = 0x40f2b7;
    hwaddr paddr_page = cpu_get_phys_page_debug( RMC_private_cpu, vaddr_page );
    hwaddr phys_addr = paddr_page | (guest_virt & ~TARGET_PAGE_MASK);
 
    env->cr[3] = temp_cr3;
    env->hflags = temp_hflags;
    */
    
    return 0x0; // FIXME
}

/* Takes the guest physical address of a guest buffer, reads the guest
 * virtual address written in there and translates that into the guest
 * physical address of the second level buffer */
hwaddr RMC_run(void *RMC_host_addr) {
	RMC_initialised = 1;
	hwaddr curr_guest_phys = 0;
	curraddr = malloc(256);
	strcpy(curraddr, (char *)RMC_host_addr);
	if (prevaddr != NULL) {
		if (strcmp(prevaddr, curraddr) != 0) {
			prevaddr = malloc(strlen(curraddr)+1);
			strcpy(prevaddr, curraddr);
			curr_guest_phys = RMC_parse(prevaddr);
			#if defined(RMC_DEBUG)
				printf("shared buf translated address as %016lX\n", curr_guest_phys);
			#endif
		}
	} else {
		free(prevaddr);
		prevaddr = malloc(strlen(curraddr)+1);
		strcpy(prevaddr, curraddr);
		curr_guest_phys = RMC_parse(prevaddr);
		#if defined(RMC_DEBUG)
			printf("RMC_cq translated address as %016lX\n", curr_guest_phys);
		#endif
	}
	free(curraddr);
	return curr_guest_phys;
}

/* Parses the string information in a buffer into a guest virtual address
 * and translates this into a guest physical */
hwaddr RMC_parse(char *curraddr) {
	unsigned long long int curr_virt_addr = strtoll(curraddr, NULL, 16);
	hwaddr curr_guest_phys = 0;
	curr_guest_phys = RMC_translate((vaddr) curr_virt_addr);
	return curr_guest_phys;
}

/* Sets init values of all variables for the completion queue. It's passed
 * the host virtual address of the completion queue so the location can be 
 * set correctly */
void RMC_cq_init(size_t qp_id, void *RMC_host_virt_cq) {
	//set private variables
	cq_SR = 1;
	cq_head = 0;

    rmc_cq_t* cq = &(my_qps.cq_arr[qp_id]);
	
	//set location of cq members and some init values
	(cq->tail) = RMC_host_virt_cq;
	(cq->SR) = RMC_host_virt_cq + 1;
	*(cq->tail) = 0;
	*(cq->SR) = 1;
	int i;
	for (i=0; i<MAX_NUM_WQ; i++) {
		(cq->q[i]) = RMC_host_virt_cq + 2 + i;
		(cq->q[i])->SR = 0;
	}
}

/* Sets init values of all variables for the work queue. It's passed
 * the host virtual address of the work queue so the location can be 
 * set correctly */
void RMC_wq_init(size_t qp_id, void *RMC_host_virt_wq) {
	//set private variables
	wq_tail = 0;
	wq_SR = 1;
	
    rmc_wq_t* wq = &(my_qps.wq_arr[qp_id]);

	//set location of wq members
	(wq->head) = RMC_host_virt_wq;
	(wq->SR) = RMC_host_virt_wq + 1;
	int i;
	for (i = 0; i < MAX_NUM_WQ; i++) {
		(wq->q[i]) = RMC_host_virt_wq + 2 + i * 16;
	}
}

/* All pipeline functions are called and advanced. The pipelines
 * therefore work sequentially and do not need an arbiter for memory
 * accesses at this moment.
 * The do-while loop is here to make the execution faster and not so 
 * dependent on callbacks outside of our control.
 * Future parallization might want to instantiate threads for these three
 * pipelines so they can work in parallel */
void RMC_advance_pipelines(void) {
	do {
		RMC_RCP_pipeline();
		RMC_RGP_pipeline();
		RMC_RRPP_pipeline();
	} while (RCP_current_state != RCP_Decode || RGP_current_state != RGP_Poll || RRPP_current_state != RRPP_Decode);
}

/* An entry to the Inflight Transaction Table (ITT) is generated. The ITT
 * is an array with MAX_NUM_WQ rows and 4 columns. 
 * first field: how many packets will be coming to fulfill request
 * second field: how many packets have been confirmed
 * third field: local buffer address for storing data
 * fourth field: how big was the original offset (before request 
 * 	unrolling may have changed the offset recorded in the packet) */
void generate_ITT_entry(int current_tid, int offset_request, uint64_t buf_addr, uint64_t baseline_offset) {
	ITT[current_tid][0] = offset_request;
	ITT[current_tid][1] = 0;
	ITT[current_tid][2] = buf_addr;
	ITT[current_tid][3] = baseline_offset;
}

/* Called when a work order is completed. The SR bit and the tid are set
 * so the application knows which work queue entry has been fulfilled.
 * The head of the completion queue is then moved */
void generate_CQ_entry(int current_tid) {
    rmc_cq_t* cq = &(my_qps.cq_arr[0]); // FIXME: change API to support multiple qps
	(cq->q[cq_head])->SR = cq_SR;
	(cq->q[cq_head])->tid = (uint8_t) current_tid;
	
	++cq_head;
	if (cq_head == MAX_NUM_WQ) {
		cq_head = 0;
		cq_SR = !cq_SR;
	}
}

/* Called by the Network Interface Card after Identification via IP-4 Protocol
 * header field equal to RMC_ip_protocol_definition. This function looks into
 * the Operation field of the RMC packet and determines into which packet buffer
 * it should be copied, so the respective pipeline can pick it up. */
void RMC_remote_msg(const uint8_t *buf) {
	const uint8_t *i_buf=buf;
	int i;
	//ignore momentarily unimportant fields
	for (i = 0; i < 34; ++i) {
		++i_buf;
	}
	//distinguish between ops
	uint8_t op = *i_buf;
	if (op == RMC_OP_RREADCOMP || op == RMC_OP_RWRITECOMP) {
		RMC_buffer_push(&RCP_buffer, (void *)buf);
	} else {
		if (op != RMC_OP_REJECTION) {
			RMC_buffer_push(&RRPP_buffer, (void *)buf);
		} else {
			int current_tid = (*((write_completion_eth_frame_t *) buf)).tid;
			int done = RCP_update_ITT_entry(current_tid);
			if (done == 1) {
				generate_CQ_entry(current_tid);
				printf("This RMC has received rejection messages. The context id was wrong for the entry at %u.\n", current_tid);
			}
		}
	}
}

/* Called by the Network Card during initialization so the RMC knows the
 * NIC of the system its operating in */
void RMC_initialize_NIC(void *s) {
	RMC_NICState = s;
}

/* Called by the Network Card during initialization so the RMC knows the
 * MAC address of the system its operating in */
void RMC_initialize_MACAddr(MACAddr macaddr) {
	RMC_macaddr = macaddr;
}

/* Called during initialization to store an image of a cpu, so the RMC
 * can translate addresses without a guest CPU currently being active */
void RMC_initialize_cpu(void) {
    PTH_UPDATE_CONTEXT;
	RMC_private_cpu = PTH(current_cpu);
}

/* Store the guest physical address of the shared space that the RMC can 
 * use to manage incoming remote requests. This address is gained by 
 * translating a guest virtual address stored in a second level buffer */
void RMC_initialize_shared_space(hwaddr sharedbuf) {
    /*
	local_shared_space = sharedbuf;
	void *temp_cid_buffer = malloc(1);
	cpu_physical_memory_read(local_shared_space, temp_cid_buffer, 1);
	uint8_t temp_cid = (uint8_t) *((const uint8_t *)temp_cid_buffer);
	if (temp_cid != local_cid) {
		printf("The startup cid and userapp for this RMC do not match up!\n");
		printf("Userapp was configured with cid %u, while this RMC is in context %u\n", temp_cid, local_cid);
	}
    */
}

/* Sets all fields of the IP header that are the same every time */
void RMC_set_ipv4_header_static_part(eth_ip_frame__header_t *header) {
	(*header).ip_v_hl = 0x45;
	(*header).ip_tos = 0;
	memset(&((*header).ip_id), 0, 2);
	memset(&((*header).ip_off), 0, 2);
	(*header).ip_ttl = 20;
	(*header).ip_p = RMC_ip_protocol_definition;
	memset(&((*header).ip_sum), 0, 2);
}

/* Sets the IP header, i.e. the static part, source, destination, length
 * (depends on the type of RMC packet) and calls the function to compute
 * the checksum. */
void RMC_set_ipv4_header(eth_ip_frame__header_t *header, uint8_t dest, uint8_t op) {
	RMC_set_ipv4_header_static_part(header);
    uint8_t src[] = {0x0A,0x01,(uint8_t)local_nid,0x01};
    uint8_t dst[] = {0x0A,0x01, dest,0x01};
    memcpy(&((*header).ip_src), &src, 4);
    memcpy(&((*header).ip_dst), &dst, 4);
    uint8_t len[] = {0x00,0x2E};
	uint16_t num;
	switch(op) {
		case RMC_OP_RREAD:
			memcpy(&((*header).ip_len), &len, 2);
			break;
		case RMC_OP_RREADCOMP:
			num = CACHE_LINE_SIZE_BYTE + 30;
			(*header).ip_len[1] = (uint8_t) num;
			(*header).ip_len[0] = (uint8_t) (num>>8);
			break;
		case RMC_OP_RWRITE:
			num = CACHE_LINE_SIZE_BYTE + 32;
			(*header).ip_len[1] = (uint8_t) num;
			(*header).ip_len[0] = (uint8_t) (num>>8);
			break;
		case RMC_OP_RWRITECOMP:
			memcpy(&((*header).ip_len), &len, 2);
			break;
	}
    uint16_t checksum = ip_checksum(&(header->ip_v_hl), 20);
    (*header).ip_sum[1] = (uint8_t) checksum;
	(*header).ip_sum[0] = (uint8_t) (checksum>>8);
}

/*Helper function to convert from uint64_t to uint8_t[8] */
void uint64_To_uint8 (uint8_t *buf, uint64_t var, uint32_t lowest_pos) {
    buf[lowest_pos]     =   (var & 0x00000000000000FF) >> 0 ;
    buf[lowest_pos+1]   =   (var & 0x000000000000FF00) >> 8 ;
    buf[lowest_pos+2]   =   (var & 0x0000000000FF0000) >> 16 ;
    buf[lowest_pos+3]   =   (var & 0x00000000FF000000) >> 24 ;
    buf[lowest_pos+4]   =   (var & 0x000000FF00000000) >> 32 ;
    buf[lowest_pos+5]   =   (var & 0x0000FF0000000000) >> 40 ;
    buf[lowest_pos+6]   =   (var & 0x00FF000000000000) >> 48 ;
    buf[lowest_pos+7]   =   (var & 0xFF00000000000000) >> 56 ;
}

/* Helper function to convert from uint8_t[8] to uint64_t */
uint64_t uint8_To_uint64 (uint8_t *var, uint32_t lowest_pos) {
    return  (((uint64_t)var[lowest_pos+7]) << 56) | 
            (((uint64_t)var[lowest_pos+6]) << 48) |
            (((uint64_t)var[lowest_pos+5]) << 40) | 
            (((uint64_t)var[lowest_pos+4]) << 32) |
            (((uint64_t)var[lowest_pos+3]) << 24) | 
            (((uint64_t)var[lowest_pos+2]) << 16) |
            (((uint64_t)var[lowest_pos+1]) << 8)  | 
            (((uint64_t)var[lowest_pos])   << 0);
}

/* Computes the checksum of the IP header. It has to be passed the 
 * pointer to the start of the IP header (not the combined Ethernet-IP
 * header!) and the header length. The checksum has to zero before
 * calling this function. Return value is already in network order */
uint16_t ip_checksum(void* vdata,size_t length) {
    char* data=(char*)vdata;
    uint32_t checksum=0xffff;
    size_t i;
    for (i=0;i+1<length;i+=2) {
        uint16_t word;
        memcpy(&word,data+i,2);
        checksum+=ntohs(word);
        if (checksum>0xffff) {
            checksum-=0xffff;
        }
    }
    if (length&1) {
        uint16_t word=0;
        memcpy(&word,data+length-1,1);
        checksum+=ntohs(word);
        if (checksum>0xffff) {
            checksum-=0xffff;
        }
    }
    return ~checksum;
}

/* Statemachine for the Request Completion Pipeline (RCP). The RCP gets
 * the completion packets from the Network Interface (stored in the
 * RCP_buffer). The pipeline has been modeled as close to the soNUMA paper
 * as possible, but does not work in parallel to the other pipelines.
 * It therefore has no need of an arbiter for memory accessses. */
void RMC_RCP_pipeline (void) {
	switch (RCP_current_state) {
		case RCP_Decode:
			/* Check RCP_buffer for new packets, if yes: decode it */
			if (RMC_buffer_empty(&RCP_buffer) == 0) {
				void *buf = RMC_buffer_popqueue(&RCP_buffer);
				const uint8_t *i_buf=buf;
				int i;
				for (i = 0; i < 34; ++i) {
					++i_buf;
				}
				uint8_t op = *i_buf;
				if (op == RMC_OP_RWRITECOMP) {
					#if defined(RMC_DEBUG)
						printf("Received write completion packet.\n");
					#endif
					RCP_frame_write = (write_completion_eth_frame_t *) buf;
					RCP_current_tid = (*RCP_frame_write).tid;
					RCP_current_state = RCP_Update_ITT;
				} else {
					#if defined(RMC_DEBUG)
						printf("Received read completion packet.\n");
					#endif
					RCP_frame_read = (read_completion_eth_frame_t *) buf;
					RCP_current_tid = (*RCP_frame_read).tid;
					RCP_current_offset = uint8_To_uint64((*RCP_frame_read).offset, 0);
					RCP_current_state = RCP_Comp_Virt;
				}
			} else {
				RCP_current_state = RCP_Decode;
			}
			break;
		case RCP_Comp_Virt:
			/* Compute guest virtual address where data should go */
			RCP_current_guest_virt = (vaddr) (ITT[RCP_current_tid][2] + RCP_current_offset - ITT[RCP_current_tid][3]);
			RCP_current_state = RCP_Translate;
			break;
		case RCP_Translate:
			/* Translate guest virtual to physical */
			RCP_current_guest_phys = RMC_translate(RCP_current_guest_virt);
			RCP_current_state = RCP_Write_Data;
			break;
		case RCP_Write_Data:
			/* Write data to computed guest physical */
			cpu_physical_memory_write(RCP_current_guest_phys, &(*RCP_frame_read).payload, CACHE_LINE_SIZE_BYTE);
			RCP_current_state = RCP_Update_ITT;
			break;
		case RCP_Update_ITT:
			/* increase count of completed requests for the corresponding
			 * ITT / Work Queue entry. Might result in a Completion Queue
			 * entry */
			;
			int done = RCP_update_ITT_entry(RCP_current_tid);
			if (done == 1) {
				RCP_current_state = RCP_Write_CQ;
			} else {
				RCP_current_state = RCP_Decode;
			}
			break;
		case RCP_Write_CQ:
			/* Generates completion queue entry */
			#if defined(RMC_DEBUG)
				printf("Generating CQ entry for WQ entry at the %u position.\n", RCP_current_tid);
			#endif
			generate_CQ_entry(RCP_current_tid);
			RCP_current_state = RCP_Decode;
			break;
	}
}

/* Called by the RCP to update an entry in the ITT. If the number of 
 * completed remote operations is equal to the number of requested
 * remote operations (Request unrolling), the return value indicates to 
 * the RCP pipeline to add a new entry in the Completion Queue. */
int RCP_update_ITT_entry(uint8_t current_tid) {
	//increment confirmed packets, check if more are expected
	ITT[current_tid][1] += 1;
	if (ITT[current_tid][0] == ITT[current_tid][1]) {
		return 1;
	}
	return 0;
}

/* Statemachine for the Request Generation Pipeline (RGP). The RGP polls
 * on the work queue for new entries.
 * The pipeline has been modeled as close to the soNUMA paper
 * as possible, but does not work in parallel to the other pipelines.
 * It therefore has no need of an arbiter for memory accessses. */
void RMC_RGP_pipeline (void) {
    // TODO: Change API to poll all connected WQ's
    rmc_wq_t* wq = &(my_qps.wq_arr[0]);
	switch (RGP_current_state) {
		case RGP_Poll:
			/* check if new valid entry at tail of work queue */
			if ((wq->q[wq_tail])->SR == wq_SR && (wq->q[wq_tail])->valid == 1) {
				RGP_current_state = RGP_Fetch;
			} else {
				RGP_current_state = RGP_Poll;
			}
			break;
		case RGP_Fetch:
			/* read the new entry */
			RGP_buf_addr = (uint64_t) (wq->q[wq_tail])->buf_addr;
			RGP_offset = (uint64_t) (wq->q[wq_tail])->offset;
			RGP_op = (wq->q[wq_tail])->op;
			RGP_dest_nid = (wq->q[wq_tail])->nid;
			RGP_cid = (wq->q[wq_tail])->cid;
			RGP_length_request = (wq->q[wq_tail])->length;
			RGP_offset_request = 0;
			#if defined(RMC_DEBUG)
				printf("\nA new wq entry at the %u position has been added.\n", wq_tail);
			#endif
			++wq_tail;
			if (wq_tail == MAX_NUM_WQ) {
				wq_tail = 0;
				wq_SR = !wq_SR;
			}
			if (RGP_op == RMC_OP_RREAD) {
				RGP_current_state = RGP_Init_ITT;
			} else {
				RGP_current_state = RGP_Translation;
			}
			break;
		case RGP_Translation:
			/* Translate buffer address to guest physical */
			RGP_phys_wq_addr = RMC_translate((vaddr) RGP_buf_addr);
			RGP_current_state = RGP_Read;
			break;
		case RGP_Read:
			/* Read data from buffer */
			RGP_wq_temp_buffer = malloc(CACHE_LINE_SIZE_BYTE * RGP_length_request);
			cpu_physical_memory_read(RGP_phys_wq_addr, RGP_wq_temp_buffer, (CACHE_LINE_SIZE_BYTE * RGP_length_request));
			RGP_current_state = RGP_Init_ITT;
			break;
		case RGP_Init_ITT:
			/* Generate a new ITT entry for this WQ entry */
			generate_ITT_entry(local_tid, RGP_length_request, RGP_buf_addr, RGP_offset);
			RGP_local_tid = local_tid;
			++local_tid;
			if (local_tid == MAX_NUM_WQ) {
				local_tid = 0;
			}
			RGP_current_state = RGP_Packet_gen;
			break;
		case RGP_Packet_gen:
			/* Here the packet is generated, i.e. all fields are set */
			if (RGP_op == RMC_OP_RREAD) {
				memcpy(&(RGP_read_packet.header.prot), &RMC_protocol_definition, 2);
				memcpy(RGP_read_packet.header.dest_addr, &MAC_basis, sizeof(MACAddr));
				memcpy(RGP_read_packet.header.source_addr, &RMC_macaddr, sizeof(MACAddr));
				RGP_read_packet.dest_nid[1] = (uint8_t) RGP_dest_nid;
				RGP_read_packet.dest_nid[0] = (uint8_t) (RGP_dest_nid>>8);
				RGP_read_packet.op = RGP_op;
				RGP_read_packet.cid = RGP_cid;
				RGP_read_packet.tid = RGP_local_tid;
				uint64_To_uint8(RGP_read_packet.offset, RGP_offset + RGP_offset_request * CACHE_LINE_SIZE_BYTE, 0);
				RMC_set_ipv4_header(&(RGP_read_packet.header), (uint8_t)RGP_dest_nid, RGP_op);
			} else {
				memcpy(&(RGP_write_packet.header.prot), &RMC_protocol_definition, 2);
				memcpy(RGP_write_packet.header.dest_addr, &MAC_basis, sizeof(MACAddr));
				memcpy(RGP_write_packet.header.source_addr, &RMC_macaddr, sizeof(MACAddr));
				RGP_write_packet.dest_nid[1] = (uint8_t) RGP_dest_nid;
				RGP_write_packet.dest_nid[0] = (uint8_t) (RGP_dest_nid>>8);
				RGP_write_packet.op = RGP_op;
				RGP_write_packet.cid = RGP_cid;
				RGP_write_packet.tid = RGP_local_tid;
				uint64_To_uint8(RGP_write_packet.offset, RGP_offset + RGP_offset_request * CACHE_LINE_SIZE_BYTE , 0);
				RMC_set_ipv4_header(&(RGP_write_packet.header), (uint8_t)RGP_dest_nid, RGP_op);
				memcpy(RGP_write_packet.payload, RGP_wq_temp_buffer+ RGP_offset_request * CACHE_LINE_SIZE_BYTE, CACHE_LINE_SIZE_BYTE);
			}
			RGP_current_state = RGP_Packet_send;
			break;
		case RGP_Packet_send:
			/* Give packet to QEMU send function */
			if (RGP_op == RMC_OP_RREAD) {
				qemu_send_packet(qemu_get_queue(RMC_NICState), (const uint8_t *)&RGP_read_packet, sizeof(read_request_eth_frame_t));
				#if defined(RMC_DEBUG)
					printf("Read request packet sent.\n");
				#endif
			} else {
				qemu_send_packet(qemu_get_queue(RMC_NICState), (const uint8_t *)&RGP_write_packet, sizeof(write_request_eth_frame_t));
				#if defined(RMC_DEBUG)
					printf("Write request packet sent.\n");
				#endif
			}
			RGP_length_request -= 1;
			++RGP_offset_request;
			if (RGP_length_request > 0) {
				RGP_current_state = RGP_Packet_gen;
			} else {
				RGP_current_state = RGP_Poll;
			}
			break;
	}
}

/* Statemachine for the Remote Request Processing Pipeline (RRPP). The 
 * RRPP gets the request packets from the Network Interface (stored in the
 * RRPP_buffer). 
 * The pipeline has been modeled as close to the soNUMA paper
 * as possible, but does not work in parallel to the other pipelines.
 * It therefore has no need of an arbiter for memory accessses. */
void RMC_RRPP_pipeline (void) {
	switch (RRPP_current_state) {
		case RRPP_Decode:
			/* Look into RRPP_buffer if new packet, if yes: decode it */
			if (RMC_buffer_empty(&RRPP_buffer) == 0) {
				void *buf = RMC_buffer_popqueue(&RRPP_buffer);
				const uint8_t *i_buf = (const uint8_t *)buf;
				int i;
				for (i = 0; i < 34; ++i) {
					++i_buf;
				}
				RRPP_op = *i_buf;
				if (RRPP_op == RMC_OP_RWRITE) {
					#if defined(RMC_DEBUG)
						printf("Received write request packet.\n");
					#endif
					RRPP_write_frame = (write_request_eth_frame_t *) buf;
				} else {
					#if defined(RMC_DEBUG)
						printf("Received read request packet.\n");
					#endif
					RRPP_read_frame = (read_request_eth_frame_t *) buf;
				}
				RRPP_current_state = RRPP_Comp_Virt;
			} else {
				RRPP_current_state = RRPP_Decode;
			}
			break;
		case RRPP_Comp_Virt:
			/* Get physical address where data should go / come from */
			if (RRPP_op == RMC_OP_RWRITE) {
				RRPP_current_offset = uint8_To_uint64((*RRPP_write_frame).offset, 0);
				if ((*RRPP_write_frame).cid != local_cid) {
					RRPP_current_state = RRPP_Send_Rejection;
				} else {
					RRPP_current_state = RRPP_RW;
				}
			} else {
				RRPP_current_offset = uint8_To_uint64((*RRPP_read_frame).offset, 0);
				if ((*RRPP_read_frame).cid != local_cid) {
					RRPP_current_state = RRPP_Send_Rejection;
				} else {
					RRPP_current_state = RRPP_RW;
				}
			}
			RRPP_current_guest_phys = gPA_ctx + RRPP_current_offset;
			break;
		case RRPP_RW:
			/* Do read or write operation */
			if (RRPP_op == RMC_OP_RWRITE) {
				cpu_physical_memory_write(RRPP_current_guest_phys, &((*RRPP_write_frame).payload), CACHE_LINE_SIZE_BYTE);
			} else {
				cpu_physical_memory_read(RRPP_current_guest_phys, &(RRPP_read_reply.payload), CACHE_LINE_SIZE_BYTE);
			}
			RRPP_current_state = RRPP_Packet_gen;
			break;
		case RRPP_Packet_gen:
			/* Here the packet is generated, i.e. all fields are set */
			if (RRPP_op == RMC_OP_RWRITE) {
				memcpy(RRPP_write_reply.header.dest_addr, &MAC_basis, sizeof(MACAddr));
				memcpy(RRPP_write_reply.header.source_addr, &RMC_macaddr, sizeof(MACAddr));
				RMC_set_ipv4_header(&(RRPP_write_reply.header), (*RRPP_write_frame).header.ip_src[3], RMC_OP_RWRITECOMP);
				memcpy(&(RRPP_write_reply.header.prot), &RMC_protocol_definition, 2);
				RRPP_write_reply.op = RMC_OP_RWRITECOMP;
				RRPP_write_reply.tid = (*RRPP_write_frame).tid;
				memcpy(&(RRPP_write_reply.offset), &((*RRPP_write_frame).offset), 8);
			} else {
				memcpy(RRPP_read_reply.header.dest_addr, &MAC_basis, sizeof(MACAddr));
				memcpy(RRPP_read_reply.header.source_addr, &RMC_macaddr, sizeof(MACAddr));
				RMC_set_ipv4_header(&(RRPP_read_reply.header), (*RRPP_read_frame).header.ip_src[3], RMC_OP_RREADCOMP);
				memcpy(&(RRPP_read_reply.header.prot), &RMC_protocol_definition, 2);
				RRPP_read_reply.op = RMC_OP_RREADCOMP;
				RRPP_read_reply.tid = (*RRPP_read_frame).tid;
				memcpy(&(RRPP_read_reply.offset), &((*RRPP_read_frame).offset), 8);
			}
			RRPP_current_state = RRPP_Packet_send;
			break;
		case RRPP_Packet_send:
			/* Give packet to QEMU send function */
			if (RRPP_op == RMC_OP_RWRITE) {
				#if defined(RMC_DEBUG)
					printf("sending write completion packet\n");
				#endif
				qemu_send_packet(qemu_get_queue(RMC_NICState), (const uint8_t *)&RRPP_write_reply, sizeof(write_completion_eth_frame_t));
			} else {
				#if defined(RMC_DEBUG)
					printf("sending read completion packet\n");
				#endif
				qemu_send_packet(qemu_get_queue(RMC_NICState), (const uint8_t *)&RRPP_read_reply, sizeof(read_completion_eth_frame_t));
			}
			RRPP_current_state = RRPP_Decode;
			break;
		case RRPP_Send_Rejection:
			memcpy(RRPP_write_reply.header.dest_addr, &MAC_basis, sizeof(MACAddr));
			memcpy(RRPP_write_reply.header.source_addr, &RMC_macaddr, sizeof(MACAddr));
			memcpy(&(RRPP_write_reply.header.prot), &RMC_protocol_definition, 2);
			RRPP_write_reply.op = RMC_OP_REJECTION;
			RRPP_write_reply.tid = (*RRPP_write_frame).tid;
			if (RRPP_op == RMC_OP_RWRITE) {
				RMC_set_ipv4_header(&(RRPP_write_reply.header), (*RRPP_write_frame).header.ip_src[3], RMC_OP_RWRITECOMP);
				RRPP_write_reply.tid = (*RRPP_write_frame).tid;
			} else {
				RMC_set_ipv4_header(&(RRPP_write_reply.header), (*RRPP_read_frame).header.ip_src[3], RMC_OP_RWRITECOMP);
				RRPP_write_reply.tid = (*RRPP_read_frame).tid;
			}
			qemu_send_packet(qemu_get_queue(RMC_NICState), (const uint8_t *)&RRPP_write_reply, sizeof(write_completion_eth_frame_t));
			RRPP_current_state = RRPP_Decode;
			break;
	}
}

/* Helper function for the Buffers in which incoming RMC packets are 
 * stored. The element in those is always a write_request packet, as it 
 * the larhgest of all send packets and so there can be no buffer 
 * overflow. All push and pop functions work with void pointers, so
 * correct casting is neccessary when using them. */
void RMC_buffer_init(buffer_t *buffer, int size) {
    buffer->size = size;
    buffer->start = 0;
    buffer->count = 0;
    buffer->element = malloc(sizeof(write_request_eth_frame_t)*size);  
}

/* Helper function to check if buffer full */
int RMC_buffer_full(buffer_t *buffer) {
    if (buffer->count == buffer->size) { 
        return 1;
    } else {
        return 0;
    }
}

/* Helper function to check if buffer empty */
int RMC_buffer_empty(buffer_t *buffer) {
    if (buffer->count == 0) {
        return 1;
    } else {
        return 0;
    }
}

/* Helper function to add element to buffer. The element is copied into
 * the buffer, we do not just set a pointer to the data! */
void RMC_buffer_push(buffer_t *buffer, void *data) {
    int index;
    if (RMC_buffer_full(buffer)) {
        printf("Buffer overflow\n");
    } else {
        index = buffer->start + buffer->count++;
        if (index >= buffer->size) {
            index = 0;
        }
        memcpy(&(buffer->element[index]), data, sizeof(write_request_eth_frame_t));
    }
}

/* Helper function to get element from buffer. It is implemented as
 * a FIFO buffer. We return just a void pointer to the data! */
void *RMC_buffer_popqueue(buffer_t *buffer) {
    void *element;
    if (RMC_buffer_empty(buffer)) {
        printf("Buffer underflow\n");
        return NULL;
    } else {
       element = &buffer->element[buffer->start];
       buffer->start++;
       buffer->count--;
       if (buffer->start == buffer->size) {
           buffer->start = 0;
       }
       return element;
    }
}

