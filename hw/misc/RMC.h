/* 
 * RMC header file for use in hosts
 * 
 * Copyright (c) 2015 Jan Alexander Wessel <Jan.Wessel@tum.de>
 * 
 * 
 * 
*/
#ifndef RMC_H
#define RMC_H

#include "qom/cpu.h"

#define MAX_NUM_WQ 128
#define CACHE_LINE_SIZE_BYTE 64
#define RMC_OP_RREAD 0
#define RMC_OP_RWRITE 1
#define RMC_OP_RREADCOMP 3
#define RMC_OP_RWRITECOMP 4
#define RMC_OP_REJECTION 5

#define RMC_DEBUG

extern int RMC_initialised;
extern uint8_t RMC_protocol_definition[];
extern uint8_t RMC_ip_protocol_definition;

typedef struct wq_entry {	
	unsigned char op : 6;
	unsigned char SR : 1;
	unsigned char valid : 1;
	unsigned long buf_addr : 42; 
	unsigned char cid : 4;
	unsigned short nid : 10;
	
	unsigned long offset : 40;
	unsigned long length : 24;
} wq_entry_t;

typedef struct cq_entry {
	volatile unsigned char SR : 1;
	volatile unsigned char success : 7;
    volatile uint8_t tid;
    volatile uint64_t recv_buf_addr : 48; // protocol v2.3, rpcvalet
    volatile uint16_t __padding : 16;
} cq_entry_t;

typedef struct rmc_wq {
	wq_entry_t *q[MAX_NUM_WQ];
	uint8_t *head;
	uint8_t *SR;
} rmc_wq_t;

typedef struct rmc_cq {
	cq_entry_t *q[MAX_NUM_WQ];
	uint8_t *tail;
	uint8_t *SR;
}rmc_cq_t;

typedef struct eth_ip_frame_header {
	uint8_t dest_addr[6];
	uint8_t source_addr[6];
	uint8_t prot[2];
    uint8_t ip_v_hl;
    uint8_t ip_tos;
    uint8_t ip_len[2];
    uint8_t ip_id[2];
    uint8_t ip_off[2];
    uint8_t ip_ttl;
    uint8_t ip_p;
    uint8_t ip_sum[2];
    uint8_t ip_src[4];
    uint8_t ip_dst[4];
}eth_ip_frame__header_t;

typedef struct read_request_eth_frame {
	eth_ip_frame__header_t header;
	uint8_t op;
	uint8_t dest_nid[2];
	uint8_t tid;
	uint8_t cid;
	uint8_t offset[8];
	uint8_t padding[13];
}read_request_eth_frame_t;

typedef struct write_request_eth_frame {
	eth_ip_frame__header_t header;
	uint8_t op;
	uint8_t dest_nid[2];
	uint8_t tid;
	uint8_t cid;
	uint8_t offset[8];
	uint8_t payload[CACHE_LINE_SIZE_BYTE];
}write_request_eth_frame_t;

typedef struct read_completion_eth_frame {
	eth_ip_frame__header_t header;
	uint8_t op;
	uint8_t tid;
	uint8_t offset[8];
	uint8_t payload[CACHE_LINE_SIZE_BYTE];
}read_completion_eth_frame_t;

typedef struct write_completion_eth_frame {
	eth_ip_frame__header_t header;
	uint8_t op;
	uint8_t tid;
	uint8_t offset[8];
	uint8_t padding[16];
}write_completion_eth_frame_t;

typedef enum RRPP_state{
	RRPP_Decode,
	RRPP_Comp_Virt,
	RRPP_RW,
	RRPP_Packet_gen,
	RRPP_Packet_send,
	RRPP_Send_Rejection
} RRPP_state_t;

typedef enum RCP_state{
	RCP_Decode,
	RCP_Comp_Virt,
	RCP_Translate,
	RCP_Write_Data,
	RCP_Update_ITT,
	RCP_Write_CQ
} RCP_state_t;

typedef enum RGP_state{
	RGP_Poll,
	RGP_Fetch,
	RGP_Translation,
	RGP_Read,
	RGP_Init_ITT,
	RGP_Packet_gen,
	RGP_Packet_send
} RGP_state_t;

typedef struct buffer {
    int size;
    int start;
    int count;
    write_request_eth_frame_t *element;
} buffer_t;



int RMC_does_exist(void);

hwaddr RMC_get_phys_wq(size_t qp_idx);
hwaddr RMC_get_phys_cq(size_t qp_idx);
hwaddr RMC_get_phys_ctx(void);

hwaddr RMC_run(void *host_virt_addr); // DEPRECATED: doesn't work
hwaddr RMC_parse(char *curraddr); // DEPRECATED: doesn't work
hwaddr RMC_translate(vaddr guest_virt);

void RMC_cq_init(size_t qp_id,void *RMC_host_virt_cq);
void RMC_wq_init(size_t qp_id,void *RMC_host_virt_wq);
void RMC_advance_pipelines(void);
void generate_ITT_entry(int current_tid, int offset_request, uint64_t buf_addr, uint64_t baseline_offset);
void generate_CQ_entry(int current_tid);
void RMC_remote_msg(const uint8_t *buf);
void RMC_initialize_NIC(void *s);
void RMC_initialize_MACAddr(MACAddr macaddr);
void RMC_initialize_shared_space(hwaddr sharedbuf);
void RMC_initialize_cpu(void);
void RMC_set_ipv4_header_static_part(eth_ip_frame__header_t *header);
void RMC_set_ipv4_header(eth_ip_frame__header_t *header, uint8_t dest, uint8_t op);
void uint64_To_uint8 (uint8_t *buf, uint64_t var, uint32_t lowest_pos);

uint64_t uint8_To_uint64 (uint8_t *var, uint32_t lowest_pos) ;
uint16_t ip_checksum(void* vdata,size_t length);

void *RMC_buffer_popqueue(buffer_t *buffer);
void RMC_buffer_push(buffer_t *buffer, void *data);
int RMC_buffer_empty(buffer_t *buffer);
int RMC_buffer_full(buffer_t *buffer);
void RMC_buffer_init(buffer_t *buffer, int size);
void RMC_RRPP_pipeline (void);
void RMC_RGP_pipeline (void);
void RMC_RCP_pipeline (void);
int RCP_update_ITT_entry(uint8_t current_tid);
#endif
