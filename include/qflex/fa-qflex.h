#ifndef FA_QFLEX_H
#define FA_QFLEX_H

#define FA_QFLEX_ROOT_DIR "/dev/shm/qflex"
#define FA_QFLEX_ARCH_STATE_SIZE ((66) * sizeof(uint32_t))

#include <stdbool.h>
#include "qemu/osdep.h"
#include "qom/cpu.h"

#define STR(x) #x
#define XSTR(s) STR(s)

#define ULONG_HEX_MAX   16
#define UINT_HEX_MAX    8

typedef enum {
    FULL, // Runs FA-QFLEX from start
    MAGIC // Runs FA-QFLEX when enabled through magic instrunction
} FAQFlexMode_t;

typedef struct FA_QFlexState_t {
    FAQFlexMode_t mode;
    bool enabled;
    bool running;
    bool simulate; // Will communicate with Scala simulator instead of FPGA
} FA_QFlexState_t;

/* (fa-qflex.c)
 */
extern FA_QFlexState_t fa_qflexState;

void fa_qflex_start(CPUState *cpu);
void fa_qflex_configure(QemuOpts *opts, Error **errp);

char* fa_qflex_read_file(const char* filename, size_t *size);
int fa_qflex_write_file(const char *filename, void* buffer, size_t size);

/* Setters and getters
 */
static inline FAQFlexMode_t fa_qflex_is_mode(void)   { return fa_qflexState.mode; }
static inline bool fa_qflex_is_enabled(void)         { return fa_qflexState.enabled; }
static inline bool fa_qflex_is_running(void)         { return fa_qflexState.running; }
static inline bool fa_qflex_is_simulate(void)        { return fa_qflexState.simulate; }
static inline void fa_qflex_update_running(bool run) { fa_qflexState.running = run; }

/* The following functions are architecture specific, so they are
 * in the architecture target directory.
 * (target/arm/fa-qflex-helper.c)
 */
/** Serializes the CPU architectural state to be transfered in a buffer.
 * @brief fa_qflex_pack_archstate
 * @param cpu   The CPU to serialize
 * @return      The buffer with the microarchitecture state data
 */
void* fa_qflex_pack_archstate(CPUState *cpu);
void fa_qflex_unpack_archstate(CPUState *cpu, uint32_t *buffer);

/**
 * @brief fa_qflex_get_page Translates from guest virtual address to host virtual address
 * NOTE: In case of FAULT, the caller should:
 *          1. Trigger transplant back from FPGA
 *          2. Reexecute instruction
 *          3. Return to FPGA when exception is done
 * @param cpu               Working CPU
 * @param addr              Guest Virtual Address to translate
 * @param acces_type        Access type: LOAD/STORE/INSTR FETCH
 * @param host_phys_page    Returns Address host virtual page associated
 * @param page_size         Returns page size
 * @return                  If 0: Success, else FAULT was generated
 */
bool fa_qflex_get_page(CPUState *cpu, uint64_t addr, MMUAccessType access_type,  uint64_t *host_phys_page, uint64_t *page_size);

#endif /* FA_QFLEX_H */
