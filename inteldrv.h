/*
 * Author:    Donald Craig
 * Professor: Prof. Carithers
 * Class:     Systems Programming
 * Date:      4/09/2022
 * File:      inteldrv.h
 * Network driver for Intel i82557.
 * 
 * References: https://github.com/torvalds/linux/blob/master/drivers/net/ethernet/intel/e100.c
 *	       Intel 8255x 10/100 Mbps Ethernet Controller Family Open Source Software Developer Manual
 *
 * Acronyms / Naming conventions used throughout:
 * (Professor probably doesn't need this but it helped keep me on track.)
 * 
 * ACK  := Acknowledgment Code
 * ARP  := Address Resolution Protocol
 * CB   := Command Block
 * CBL  := Command Block List
 * CSR  := Control Status Register
 * CU   := Command Unit
 * IA   := Individual Address
 * ICMP := Internet Control Message Protocol
 * MMIO := Memory Mapped Input/Output
 * NIC  := Network Interface Card
 * RFA  := Receive Frame Area
 * RFD  := Receive Frame Descriptor
 * RU   := Receive Unit
 * SCB  := System Control Block
 * TBD  := Transmit Buffer Descriptor
 * TCB  := Transmit Command Block
 * 
 */

#ifndef _INTEL_DRV_H
#define _INTEL_DRV_H

#include "common.h"
#include "lib.h"

// Constants
#define INTEL_VENDOR 0x8086
#define INTEL_QEMU_NIC 0x100E
#define INTEL_DSL_NIC 0x1229
#define INTEL_INT_VECTOR 0x2B
#define INTEL_CFG_LENGTH 22
#define INTEL_TCB_MAX_DATA_LEN 2600
#define INTEL_MIN_ETH_LENGTH 46
#define INTEL_MAX_ETH_LENGTH 1500
#define INTEL_ETH_HEADER_LEN 14
#define INTEL_ARP_HEADER_LEN 28
#define INTEL_RFD_SIZE 3096
#define INTEL_RX_BUF_MAX_LEN 3096

#define IPV4_HEADER_LEN 20
#define CB_COUNT 0x80
#define RFD_COUNT 1024
#define ARP_CACHE_SIZE 256
#define MAX_EEPROM_LENGTH 256
#define MAC_BYTE_LEN 6


// Data structs used

/**
 * Receive Frame Descriptor used for holding incoming data. 
 */
typedef struct {
	uint16_t status;
	uint16_t command;
	uint32_t link;
	uint32_t __pad1;
	uint16_t count;
	uint16_t size;
	uint8_t data[INTEL_RFD_SIZE];
} rfd_t;

/**
 * NIC information
 */
struct nic_data {
	struct csr * csr;
	uint8_t mac[MAC_BYTE_LEN];
	uint16_t eeprom_count;
	uint16_t eeprom[MAX_EEPROM_LENGTH];
	uint32_t avail_cb;
	struct cb* next_cb;
	struct cb* cb_to_check;
	struct {
		rfd_t* tail;
		rfd_t* head;
		rfd_t* next;
	} rfa;
	uint32_t my_ip;
};

/**
 * IPV4 data
 */
typedef struct {
	uint8_t ihl : 4;
	uint8_t version : 4;
	uint8_t dscp : 6;
	uint8_t ecn : 2;
	uint16_t total_len;
	uint16_t id;
	uint16_t flags : 3;
	uint16_t frag_offset : 13;
	uint8_t ttl;
	uint8_t protocol;
	uint16_t header_checksum;
	uint32_t src_ip;
	uint32_t dst_ip;
	uint8_t ip_data[INTEL_TCB_MAX_DATA_LEN - 5];
} ipv4_t;

/**
 * ARP cache data
 */
typedef struct {
	uint32_t ip_addr;
	uint16_t filled;
	uint8_t hw_addr[6];
} arp_data_t;

/**
 * ARP data
 */
typedef struct {
	uint16_t hw_type;
	uint16_t protocol_type;
	uint8_t hw_addr_len;
	uint8_t protocol_addr_len;
	uint16_t opcode;
	uint8_t sender_hw_addr[6];
	uint32_t sender_protocol_addr; // unaligned
	uint8_t target_hw_addr[6];
	uint32_t target_protocol_addr;
} __attribute__((packed)) arp_t;

/**
 * Ethernet frame data contents
 */
typedef union {
	uint8_t data[INTEL_TCB_MAX_DATA_LEN]; 
	ipv4_t ipv4;
	arp_t arp;
} eth_content_t;

/**
 * Ethernet Transmit packet
 */
typedef struct {
	uint8_t dst_mac[MAC_BYTE_LEN];
	uint16_t ethertype; 
	eth_content_t content;
} eth_packet_t;

/**
 * Ethernet Receive packet
 */
typedef struct {
	uint8_t dst_mac[MAC_BYTE_LEN];
	uint8_t src_mac[MAC_BYTE_LEN];
	uint16_t ethertype; 
	eth_content_t content;
} __attribute__((packed)) eth_packet_rx_t;

/**
 * MMIO Control Status Register
 */
struct csr {
	struct {
		uint8_t status;
		uint8_t stat_ack;
		uint8_t command;
		uint8_t interrupt_mask;
		uint32_t gen_ptr;
	} scb;
	uint32_t port;
	uint16_t __pad1;
	uint8_t eeprom_lo;
	uint8_t eeprom_hi;
	uint32_t mdi;
	uint32_t rx_dma_byte_count;
};

/**
 * Command Block
 */
struct cb {
	uint16_t status;
	uint16_t command;
	uint32_t link;
	union {
		uint8_t cfg[INTEL_CFG_LENGTH];
		uint8_t mac_addr[MAC_BYTE_LEN];
		struct {
			uint32_t tbd_array;
			uint16_t tcb_byte_count;
			uint8_t threshold;
			uint8_t tbd_count;
			eth_packet_t eth_packet;
		} tcb;
	} u;
};

/**
 * ICMP packet
 */
typedef struct {
	uint8_t type;
	uint8_t code;
	uint16_t chksm;
	uint32_t remainder;
} icmp_t;

//Enums

/**
 * ARP opcodes
 */
enum arp_opcode {
	arp_request = 0x0100,
	arp_reply = 0x0200
};

/**
 * Ethertype field identifiers. Byte order looks reversed from normal
 * listings but this is correct for use here.
 */
typedef enum {
	ethertype_ipv4 = 0x0008,
	ethertype_arp = 0x0608,
	ethertype_ipx = 0x3781,
	ethertype_ipv6 = 0xdd86		
} ethertype_t;

/**
 * IP protocol identifiers used in send_ipv4()
 */
typedef enum {
	ip_icmp = 0x01,
	ip_igmp = 0x02,
	ip_tcp = 0x06,
	ip_udp = 0x11
} ip_protocol_t;

/**
 * EEPROM control codes
 */
enum eeprom_lo_control {
	EESK = 0x01, // Serial clock
	EECS = 0x02, // Chip select
	EEDI = 0x04, // Serial data in
	EEDO = 0x08  // Serial data out
};

/**
 * Interrupt Status/ACK flags 
 */
enum scb_int_flags {
	ack_swi = 0x04, // SW interrupt
	ack_mdi = 0x08, // mdi read/write cycle complete
	ack_rnr = 0x10, // RU leaves ready state
	ack_cna = 0x20, // CU left active state
	ack_fr = 0x40, // finished receiving
	ack_cs_tno = 0x80 // CU finished executing CB with interrupt bit set
};

/**
 * System Control Block status flags
 */
enum scb_status_flag {
	cu_idle = 0x00,
	ru_idle = 0x00,
	ru_suspended = 0x04,
	ru_no_resources = 0x08,
	ru_ready = 0x10,
	cu_suspended = 0x40,
	cu_lpq_active = 0x80,
	ru_mask = 0x3C,
	cu_mask = 0xC0,
	cu_hqp_active = 0xC0
};


/**
 * System Control Block control opcodes  
 */
enum scb_opcodes {
	cuc_nop = 0x00,
	ruc_nop = 0x00,
	ruc_start = 0x01,
	ruc_resume = 0x02,
	ruc_recv_dma_redir = 0x03,
	ruc_abort = 0x04,
	ruc_load_header_data_size = 0x05,
	ruc_load_ru_base = 0x06,
	cuc_start = 0x10,
	cuc_resume = 0x20,
	cuc_load_dump_cnt_addr = 0x40,
	cuc_dump_stat_cnt = 0x50,
	cuc_load_cu_base = 0x60,
	cuc_dump_reset_stat_cnt = 0x70,
	cuc_cu_static_resume = 0xA0	
};

/**
 * EEPROM opcodes
 */
enum eeprom_opcodes {
	op_write = 0x05,
	op_read  = 0x06,
	op_ewds  = 0x10, // Erase/write disable
	op_ewen  = 0x13, // Erase/write enable
};

/**
 * Command block commands
 *
 * Reference: https://github.com/torvalds/linux/blob/master/drivers/net/ethernet/intel/e100.c
 */
enum cb_commands {
	cb_ia_cmd = 0x0001, // individual address
	cb_config = 0x0002, // configure
	cb_tx_cmd = 0x0004, // transmit
	cb_sf     = 0x0008, // simplified mode
	cb_i      = 0x2000, // generate interrupt
	cb_s      = 0x4000, // suspend
	cb_el     = 0x8000  // end list
};

/**
 * Command block status codes
 */
enum cb_status {
	cb_c = 0x8000,
	cb_ok = 0x2000,
	cb_u = 0x1000
};

// Methods

/**
 * Initializes network card at system start
 */
void intel_nic_init(void);

/**
 * Set IP address of the NIC
 *
 * @param ip IP address
 */
void set_ip(uint32_t ip);

/**
 * Sends an ipv4 packet.
 *
 * @param dst_ip destination IP address
 * @param data data being sent
 * @param length length of data being sent
 * @return 0 for success, otherwise failure
 */
int32_t send_ipv4(uint32_t dst_ip, void* data, uint32_t length, ip_protocol_t protocol);

/**
 * Sends an ethernet frame packet.
 * 
 * @param dest_hw_addr destination hardware address
 * @param data data to send
 * @param length length of data to send
 * @return 0 on success, otherwise failure
 */
int32_t send_packet(uint8_t dest_hw_addr[], void* data, uint16_t length, ethertype_t ethertype);

/**
 * Displays data in a readable format. 
 *
 * @param data pointer to data we want to print
 * @param length number of bytes to be read
 */
void hexdump(void* data, uint32_t length, uint32_t bytes_per_line);

/**
 * Transmit monitor for network. Recycles command blocks.
 *
 * @param arg unused
 * @return unused
 */
int32_t nic_tx_monitor(void* arg);

/**
 * Receive monitor for incoming data
 *
 * @param arg unused
 * @return unused
 */
int32_t nic_rx_monitor(void* arg);

/**
 * Enable receive unit
 */
void nic_rx_enable(void);


#endif
