/*
 * Author:    Donald Craig
 * Professor: Prof. Carithers
 * Class:     Systems Programming
 * Date:      4/08/2022
 * File:      inteldrv.c
 * Network driver for Intel i82557. 
 */

#include "inteldrv.h"
#include "pciscan.h"
#include "kmem.h"
#include "cio.h"
#include "lib.h"
#include "x86arch.h"
#include "x86pic.h"
#include "support.h"


/**
 * Static methods
 */
static uint32_t get_mem32(void* addr) __attribute__((unused));
static uint16_t get_mem16(void* addr) __attribute__((unused));
static uint8_t get_mem8(void* addr);
static void set_mem32(void* addr, uint32_t value);
static void set_mem16(void* addr, uint16_t value) __attribute__((unused));
static void set_mem8(void* addr, uint8_t value);
static void delay(uint32_t usec_10);
static char char_print(char c);
static void mac_addr_print(uint8_t mac[]);
static void eeprom_dump(struct nic_data* nic) __attribute__((unused));
static void nic_write(struct nic_data *nic);
static uint16_t eeprom_reader(struct nic_data *nic, uint16_t *addr_len, uint16_t addr); 
static void eeprom_loader(struct nic_data *nic);
static void nic_isr_install(int vector, int code);
static void cbl_ring_init(struct nic_data* nic, uint32_t num_cb);
static void rfa_nic_init(struct nic_data* nic, uint32_t num_rfd);
static void cb_release(void);
static rfd_t* set_rfa_tail(void);
static struct cb* get_next_cb(void);				
static uint16_t ip_checksum(void* ip_head, int len);
static int32_t hw_addr_store(uint32_t ip, uint8_t hw_addr[]);
static int32_t hw_addr_retrieve(uint32_t ip, uint8_t hw_addr_out[]);
static int32_t arp_test(uint32_t sender_ip_addr) __attribute__((unused));		
static int32_t arp_snd_reply(uint32_t target_ip_addr, uint8_t target_hw_addr[]);
static int32_t arp_snd_request(uint32_t target_ip_addr);
static int32_t arp_send(uint32_t sender_ip_addr, uint32_t target_ip_addr, uint8_t target_hw_addr[], enum arp_opcode opcode);

// Network interface information
static struct nic_data _nic;

// Ip->mac entries
static arp_data_t* _arp_cache;

/**
 * Delay function, approx 10us
 *
 * @param usec_10 # of 10us periods to delay
 */
static void delay(uint32_t usec_10) {
	for (; usec_10 > 0; usec_10--) {
		for (uint32_t i = 0; i < 4000; ++i) { } 
	}
}

/**
 * Ensures character is always printable. If normally printable
 * that character gets printed, otherwise it is replaced with
 * a dash.
 *
 * @param ch character to replace if necessary
 * @return printable character
 */
static char char_print(char ch) {
	if(ch  > 31 && ch < 127) {
		return ch;
	}
	else {
		return '-';
	}
}

/**
 * Display MAC address (XX:XX:XX:XX:XX:XX)
 *
 * @param mac uint8_t[6] MAC address
 */
static void mac_addr_print(uint8_t mac[]) {
	__cio_printf("%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

/**
 * Dump contents of the EEPROM cache held in memory
 *
 * @param nic containing EEPROM
 */
static void eeprom_dump(struct nic_data* nic) {
	for(uint32_t i = 0; i < nic->eeprom_count; i++) {
		if(i % 6 == 0)
			__cio_printf("\neep:");
		__cio_printf(" [%d]=0x%04x", i, nic->eeprom[i]);
	}
}

/**
 * Write MMIO struct to the nic
 *
 * @param nic card to write to
 */
static void nic_write(struct nic_data *nic) {
	(void) get_mem8(&nic->csr->scb.status);
}

/**
 * Reads contents of the EEPROM
 * Reference: https://github.com/torvalds/linux/blob/master/drivers/net/ethernet/intel/e100.c
 *
 * @param nic location of the EEPROM 
 * @param addr_len 
 * @param addr
 * @return EEPROM data
 */
static uint16_t eeprom_reader(struct nic_data *nic, uint16_t *addr_len, uint16_t addr) {
	uint32_t cmd_addr_data;
	uint16_t data = 0;
	uint8_t ctrl;
	int32_t i;

	cmd_addr_data = ((op_read << *addr_len) | addr) << 16;
	/* Chip select */
	set_mem8(&nic->csr->eeprom_lo, EECS | EESK);

	nic_write(nic); delay(1);
	
	/* Bit-bang to read word from eeprom */
	for (i = 31; i >= 0; i--) {
		ctrl = (cmd_addr_data & (1 << i)) ? EECS | EEDI : EECS;
		set_mem8(&nic->csr->eeprom_lo, ctrl);
		nic_write(nic); delay(1);

		set_mem8(&nic->csr->eeprom_lo, ctrl | EESK);
		nic_write(nic); delay(1);

		/* Eeprom drives a dummy zero to EEDO after receiving
		 * complete address.  Use this to adjust addr_len. */
		ctrl = get_mem8(&nic->csr->eeprom_lo);
		if (!(ctrl & EEDO) && i > 16) {
			*addr_len -= (i - 16);
			i = 17;
		}

		data = (data << 1) | (ctrl & EEDO ? 1 : 0);
	}
	/* Chip deselect */
	set_mem8(&nic->csr->eeprom_lo, 0);
	nic_write(nic); delay(1);
	return data;
}

/**
 * Load entire EEPROM into memory & validate checksum
 * Reference: https://github.com/torvalds/linux/blob/master/drivers/net/ethernet/intel/e100.c
 * 
 * @param nic specifies location EEPROM cache to be stored at
 */
static void eeprom_loader(struct nic_data *nic) {
	uint16_t addr, addr_len = 8, checksum = 0;

	/* Try reading with an 8-bit addr len to discover actual addr len */
	eeprom_reader(nic, &addr_len, 0);
	nic->eeprom_count = 1 << addr_len;
	for(addr = 0; addr < nic->eeprom_count; addr++) {
		nic->eeprom[addr] = eeprom_reader(nic, &addr_len, addr);
		if(addr < nic->eeprom_count - 1) {
			checksum += nic->eeprom[addr];
		}
	}
	__cio_printf("\n");

	/* The checksum, stored in the last word, is calculated such that
	 * the sum of words should be 0xBABA */
	if((0xBABA - checksum) != nic->eeprom[nic->eeprom_count - 1]) {
		__cio_printf("EEPROM corrupted\n");
	}
}


/**
 * NIC interrupt handler 
 *
 * @param vector interrupt vector
 * @param code interrupt code 
 */
static void nic_isr_install(int vector, int code) {
	(void) vector; (void) code;
	uint8_t stat_ack = get_mem8(&_nic.csr->scb.stat_ack);
	if(stat_ack & ack_fr) {
		rfd_t* rfd = _nic.rfa.next;
		rfd->count &= ~0x8000; 
		rfd->count &= ~0x4000; 
		_nic.rfa.next = (rfd_t*) rfd->link;
	}

	// acknowledge status bits
	set_mem8(&_nic.csr->scb.stat_ack, ~0);
	nic_write(&_nic);

	if(vector >= 0x20 && vector < 0x30) {
		__outb(PIC_PRI_CMD_PORT, PIC_EOI);
		if(vector > 0x27) {
			__outb(PIC_SEC_CMD_PORT, PIC_EOI);
		}
	}
}

/**
 * Create linked ring of command blocks to initialize the nic
 *
 * @param nic nic to initialize 
 * @param num_cb command block quantity
 */
static void cbl_ring_init(struct nic_data* nic, uint32_t num_cb) {
	struct cb* first = (struct cb*) _km_page_alloc(1);
	struct cb* curr = first;
	curr->status = 0;
	for(uint32_t i = 0; i < num_cb; i++) {
		curr->link = (uint32_t) _km_page_alloc(1);
		curr = (struct cb*) curr->link;
		curr->status = 0;
	}
	curr->link = (uint32_t) first; 

	nic->avail_cb = num_cb;
	nic->next_cb = first;
	nic->cb_to_check = first;
}

/**
 * Create singly linked list of receive frame descriptors to initialize the nic
 * 
 * @param nic nic to initialize 
 * @param num_rfd receive frame descriptors quantity
 */
static void rfa_nic_init(struct nic_data* nic, uint32_t num_rfd) {
	rfd_t* curr = (rfd_t*) _km_page_alloc(1);
	nic->rfa.head = curr;
	nic->rfa.next = curr;
	for(uint32_t i = 0; i < num_rfd; i++) {
		__memset(curr, 0, 16); // zero out the RFD header
		curr->size = INTEL_RFD_SIZE;
		curr->command = 1 << 3; // simplified mode
		curr->link = (uint32_t) _km_page_alloc(1);
		curr = (rfd_t*) curr->link;
	}
	__memset(curr, 0, 16); // zero out the RFD header
	curr->size = INTEL_RFD_SIZE;
	curr->command = 0x8000 | (1 << 3); // last in the list and simplified
	nic->rfa.tail = curr;
}

/**
 * Releases command blocks  
 */
static void cb_release() {
	while(_nic.cb_to_check->status & cb_c) {
		// __cio_printf("NIC: released command block: %08x\n", (uint32_t) _nic.cb_to_check);
		_nic.cb_to_check->status &= ~cb_c; // unset complete flag
		_nic.cb_to_check->command &= 0;
		_nic.avail_cb++; 
		_nic.cb_to_check = (struct cb*) _nic.cb_to_check->link;
	}
}

/**
 * Set tail of RFA list
 *
 * @return tail of RFA list
 */
static rfd_t* set_rfa_tail() {
	rfd_t* curr = (rfd_t*) _km_page_alloc(1);
	__memset(curr, 0, 16); 
	curr->size = INTEL_RFD_SIZE;
	curr->command = 0x8000 | (1 << 3);
	return curr;
}

/**
 * Retrieves the next available command block from the nic_data
 *
 * @return next available command block, NULL if none exist
 */
static struct cb* get_next_cb() {
	if(!_nic.avail_cb) {
		cb_release();
		if(!_nic.avail_cb) {
			__cio_printf("NIC: Command Blocks not available :(\n");
			return NULL;
		}
	}
	struct cb* cb = _nic.next_cb;
	_nic.next_cb = (struct cb*) cb->link;
	_nic.avail_cb--;
	return cb;
}

/**
 * Send ARP test packet with a given IP
 *
 * @param sender_ip_addr IP address of sender
 * @return 0 on success, otherwise failure
 */
static int32_t arp_test(uint32_t sender_ip_addr) {
	uint8_t target_hw_addr[6];
	target_hw_addr[0] = 0xFF;
	target_hw_addr[1] = 0xFF;
	target_hw_addr[2] = 0xFF;
	target_hw_addr[3] = 0xFF;
	target_hw_addr[4] = 0xFF;
	target_hw_addr[5] = 0xFF;
	return arp_send(sender_ip_addr, sender_ip_addr, target_hw_addr, arp_request);
}

/**
 * Sends an ARP reply
 *
 * @param target_ip_addr IP to send ARP reply to
 * @param target_hw_addr HW addr to send ARP reply to
 * @return
 */
static int32_t arp_snd_reply(uint32_t target_ip_addr, uint8_t target_hw_addr[]) {
	return arp_send(_nic.my_ip, target_ip_addr, target_hw_addr, arp_reply);
}

/**
 * Sends an ARP request
 *
 * @param target_ip_addr who to send the ARP request to
 * @return
 */
static int32_t arp_snd_request(uint32_t target_ip_addr) {
	__cio_printf("Sending arp request\n");
	uint8_t zero_mac[6] = {0,0,0,0,0,0};
	return arp_send(_nic.my_ip, target_ip_addr, zero_mac, arp_request);
}

/**
 * Sends an ARP packet
 *
 * @param sender_ip_addr sender's IP address
 * @param target_ip_addr target's IP address
 * @param target_hw_addr target's HW address
 * @param opcode type of arp (request/reply)
 * @return 0 on success, otherwise failure
 */
static int32_t arp_send(uint32_t sender_ip_addr, uint32_t target_ip_addr, uint8_t target_hw_addr[], enum arp_opcode opcode) {
	arp_t arp;
	arp.hw_type = 0x0100; // ethernet in NBO
	arp.protocol_type = 0x0008; // IPv4 inn NBO
	arp.hw_addr_len = 0x06; // ethernet HW addr length
	arp.protocol_addr_len = 0x04; // IPv4 addr length
	arp.opcode = opcode; // request in NBO

	__memcpy(&arp.sender_hw_addr, _nic.mac, MAC_BYTE_LEN);
	arp.sender_protocol_addr = __builtin_bswap32(sender_ip_addr);
	arp.target_protocol_addr = __builtin_bswap32(target_ip_addr);
	__memcpy(&arp.target_hw_addr, target_hw_addr, MAC_BYTE_LEN);
	send_packet(target_hw_addr, &arp, sizeof(arp_t), ethertype_arp);
	return 0;
}

int32_t send_packet(uint8_t dest_hw_addr[], void* data, uint16_t length, ethertype_t ethertype) {
	if(length > INTEL_MAX_ETH_LENGTH) {
		return -1;
	}

	struct cb* cb = get_next_cb();
	if(cb == NULL) {
		return -1;
	}

	uint32_t tx_length = (length < INTEL_MIN_ETH_LENGTH) ? INTEL_MIN_ETH_LENGTH : length;
	// ensure minimum frame size is met by padding from length of user data
	// through 46 bytes
	for (int i = length; i <= INTEL_MIN_ETH_LENGTH; i++) {
		cb->u.tcb.eth_packet.content.data[i] = 0;
	}

	cb->command = cb_el | cb_sf | cb_tx_cmd;
	cb->u.tcb.tbd_array = 0xFFFFFFFF; // for simplified mode, tbd is 1's, data is in tcb
	cb->u.tcb.tcb_byte_count = (tx_length + INTEL_ETH_HEADER_LEN)| 1 << 15; // bit15 = EOF flag
	cb->u.tcb.threshold = 1; // transmit once you have (1 * 8 bytes) in the queue
	cb->u.tcb.tbd_count = 0;
	__memcpy(cb->u.tcb.eth_packet.dst_mac, dest_hw_addr, MAC_BYTE_LEN);
	cb->u.tcb.eth_packet.ethertype = ethertype;
	__memcpy(cb->u.tcb.eth_packet.content.data, data, length);
	uint8_t status = get_mem8(&_nic.csr->scb.status);
	if(((status & cu_mask) == cu_idle) || ((status & cu_mask) == cu_suspended)) {
		set_mem32(&_nic.csr->scb.gen_ptr, (uint32_t) cb);
		set_mem8(&_nic.csr->scb.command, cuc_start);
		nic_write(&_nic);
	}
	return 0;
}

/**
 * IP header checksum calculator
 *
 * @param ip_header pointer to IP header
 * @param len header length
 * @return header checksum
 */
static uint16_t ip_checksum(void* ip_header, int len) {
	uint32_t sum = 0;
	const uint16_t *ip1 = ip_header;
	while(len > 1){
		sum += *ip1++;
		if(sum & 0x80000000) {
			sum = (sum & 0xFFFF) + (sum >> 16);
		}
		len -= 2;
	}
	if(len) {
		sum += (uint16_t) *(uint8_t*) ip_header;
	}
	while(sum >> 16) {
		sum = (sum & 0xFFFF) + (sum >> 16);
	}
	return ~sum;
}

/**
 * Stores hardware address in arp cache
 *
 * @param ip IP address
 * @param hw_addr hardware address
 * @return 0 for success, -1 for failure
 */
static int32_t hw_addr_store(uint32_t ip, uint8_t hw_addr[]) {
	uint32_t hash = ip % ARP_CACHE_SIZE;
	for(int index = hash, count = 0; count < ARP_CACHE_SIZE; index++, count++) {
		if(index == ARP_CACHE_SIZE) index = 0; // wrap around
		// __cio_printf("trying to insert 0x%08x at _arp_cache[%d]\n", ip, index);
		if(_arp_cache[index].ip_addr == ip || !_arp_cache[index].filled) { // update or add
			// __cio_printf("inserting at _arp_cache[%d]\n", index);
			_arp_cache[index].ip_addr = ip;
			_arp_cache[index].filled = 1;
			__memcpy(_arp_cache[index].hw_addr, hw_addr, 6);
			return 0;
		}
	}
	return -1;
}

/**
 * Retrieve hardware address from an IP
 *
 * @param ip IP to get hardware address of
 * @param hw_addr_out out parameter for the hw address
 * @return 0 on success, -1 for failure
 */
static int32_t hw_addr_retrieve(uint32_t ip, uint8_t hw_addr_out[]) {
	uint32_t hash = ip % ARP_CACHE_SIZE;
	for(int i = 0; i < 5; i++) {
		for(int index = hash, count = 0; count < ARP_CACHE_SIZE; index++, count++) {
			if(index == ARP_CACHE_SIZE) index = 0; // wrap around
			if(_arp_cache[index].ip_addr == ip && _arp_cache[index].filled) {
				__memcpy(hw_addr_out, _arp_cache[index].hw_addr, 6);
				return 0;
			}
		}
		arp_snd_request(ip);
		sleep(2000);
	}
	return -1;
}

int32_t send_ipv4(uint32_t dst_ip, void* data, uint32_t length, ip_protocol_t protocol) {
	uint8_t hw_addr[6];
	if(hw_addr_retrieve(dst_ip, hw_addr)) {
		__cio_printf("NIC: Couldn't find specified IP\n");
		return -1;
	}
	ipv4_t ipv4;
	ipv4.version = 4;
	ipv4.ihl = 5;
	ipv4.dscp = 0;
	ipv4.ecn = 0;
	uint16_t ip_length = length + IPV4_HEADER_LEN;
	ipv4.total_len = __builtin_bswap16(ip_length);
	ipv4.id = 0;
	ipv4.flags = 0;
	ipv4.frag_offset = 0;
	ipv4.ttl = 64;
	ipv4.protocol = protocol;
	ipv4.header_checksum = 0;
	ipv4.src_ip = __builtin_bswap32(_nic.my_ip);
	ipv4.dst_ip = __builtin_bswap32(dst_ip);
	ipv4.header_checksum = ip_checksum(&ipv4, IPV4_HEADER_LEN);

	__memcpy(ipv4.ip_data, data, length);
	return send_packet(hw_addr, &ipv4, ip_length, ethertype_ipv4);
}

void set_ip(uint32_t ip) {
	_nic.my_ip = ip;
}

void hexdump(void* data, uint32_t length, uint32_t bytes_per_line) {
	uint8_t* data_ = (uint8_t*) data;
	__cio_printf("hexdump(data=0x%08x, length=%d, bytes_per_line=%d):\n", (uint32_t) data, length, bytes_per_line);
	for(uint32_t i = 0; i < length; i+=bytes_per_line) {
		__cio_printf("[%5x]", i);
		for(uint32_t n = 0; n < bytes_per_line; n++) {
			if(n % 2 == 0) {
				__cio_printf(" ");
			}
			if(n + i >= length) {
				__cio_printf("  ");
			}
			else {
				__cio_printf("%02x", data_[n + i]);
			}
		}
		__cio_printf("  |");
		for(uint32_t n = 0; n < bytes_per_line; n++) {
			if(n % 8 == 0) {
				__cio_printf(" ");
			}
			if(n + i >= length) {
				__cio_printf(" ");
			}
			else {
				__cio_printf("%c", char_print(data_[n + i]));
			}
		}
		__cio_printf(" |\n");
	}
}

int32_t nic_tx_monitor(void* arg) {
	(void) arg;
	for(;;) {
		cb_release();
		sleep(500);
	}
}

int32_t nic_rx_monitor(void* arg) {
	(void) arg;
	nic_rx_enable();
	for(;;) {
		while(_nic.rfa.head != _nic.rfa.next) {
			rfd_t* rfd = _nic.rfa.head;
			uint16_t byte_count = rfd->count & 0x3FFF;
			eth_packet_rx_t* packet = (eth_packet_rx_t*) rfd->data;
			
			if(packet->ethertype == ethertype_arp) {
				if(packet->content.arp.opcode == arp_request) {
					__cio_printf("NIC: ARP request received\n");
					arp_snd_reply(packet->content.arp.sender_protocol_addr, packet->content.arp.sender_hw_addr);
					hw_addr_store(__builtin_bswap32(packet->content.arp.sender_protocol_addr), packet->content.arp.sender_hw_addr);
				}
				else if(packet->content.arp.opcode == arp_reply) {
					__cio_printf("NIC: ARP reply received\n");
					hw_addr_store(__builtin_bswap32(packet->content.arp.sender_protocol_addr), packet->content.arp.sender_hw_addr);
				}
			}
			else if(packet->ethertype == ethertype_ipv4) {
				switch(packet->content.ipv4.protocol) {
					case ip_icmp:
					{
						icmp_t* icmp = (icmp_t*) &packet->content.ipv4.ip_data;
						__cio_printf("NIC: received ICMP - type=%d, code=%d\n", icmp->type, icmp->code);
					}
						break;

					case ip_igmp:
						__cio_printf("NIC: received IGMP\n");
						break;

					case ip_tcp:
						__cio_printf("NIC: received TCP\n");
						break;

					case ip_udp:
						__cio_printf("NIC: received UDP\n");
						break;

					default:
						break;
				}
			}
			else { // if(packet->ethertype < 1500) {

			}

			hexdump(rfd->data, byte_count, 16);

			_nic.rfa.head = (rfd_t*) rfd->link;
			_km_page_free(rfd);
			_nic.rfa.tail->link = (uint32_t) set_rfa_tail();
			_nic.rfa.tail->command &= ~(0x8000); 
			_nic.rfa.tail = (rfd_t*) _nic.rfa.tail->link;
		} 
	} 
	return 0; 
}

void intel_nic_init() {
	uint8_t bus, slot;
	if(pci_get_device(&bus, &slot, INTEL_VENDOR, INTEL_DSL_NIC) >= 0) {	
		__cio_printf("NIC: Intel 8255x NIC found - bus: %d, slot: %d\n", bus, slot);
	}
	else {
		__cio_printf("NIC: Intel NIC not found\n");
		return;
	}

	_nic.csr = (struct csr *) pci_cfg_read(bus, slot, 0, PCI_CSR_MEM_BASE_ADDR_REG);
	__cio_printf("NIC: CSR MMIO base addr: 0x%08x\n", (uint32_t) _nic.csr);

	__cio_printf("NIC: Loading data from EEPROM...\n");
	eeprom_loader(&_nic);

	// Setup MAC address 
	_nic.mac[0] = (uint8_t) (_nic.eeprom[0] & 0x00FF);
	_nic.mac[1] = (uint8_t) (_nic.eeprom[0] >> 8);
	_nic.mac[2] = (uint8_t) (_nic.eeprom[1] & 0x00FF);
	_nic.mac[3] = (uint8_t) (_nic.eeprom[1] >> 8);
	_nic.mac[4] = (uint8_t) (_nic.eeprom[2] & 0x00FF);
	_nic.mac[5] = (uint8_t) (_nic.eeprom[2] >> 8);
	__cio_printf("NIC: Hardware MAC Address: ");
	mac_addr_print(_nic.mac);
	__cio_printf("\n");

	// Set base CU and RU to 0
	set_mem32(&_nic.csr->scb.gen_ptr, 0);
	set_mem8(&_nic.csr->scb.command, cuc_load_cu_base);
	nic_write(&_nic);
	set_mem32(&_nic.csr->scb.gen_ptr, 0);
	set_mem8(&_nic.csr->scb.command, ruc_load_ru_base);
	nic_write(&_nic);

	// Setup interrupt handler
	__install_isr(INTEL_INT_VECTOR, nic_isr_install);

	// Setup CBL, RFA, rx buffer, and ARP cache
	__cio_printf("NIC: Initializing CBL\n");
	cbl_ring_init(&_nic, CB_COUNT);
	__cio_printf("NIC: Initializing RFA\n");
	rfa_nic_init(&_nic, RFD_COUNT);
	__cio_printf("NIC: Initializing ARP cache\n");
	_arp_cache = (arp_data_t*) _km_page_alloc(1);
	__memset(_arp_cache, 0, 4096);

	__cio_printf("NIC: Initial configuration\n");
	struct cb* init_config_cb = get_next_cb();
	init_config_cb->command = cb_config;
	init_config_cb->u.cfg[0] = 16; 
	init_config_cb->u.cfg[1] = 8;
	init_config_cb->u.cfg[2] = 0;
	init_config_cb->u.cfg[3] = 0;
	init_config_cb->u.cfg[4] = 0;
	init_config_cb->u.cfg[5] = 0;
	init_config_cb->u.cfg[6] = 
		1 << 7 |
		1 << 6 |
		1 << 5 |
		1 << 4 |
		1 << 2 |
		1 << 1;
	init_config_cb->u.cfg[7] = 1 << 1;
	init_config_cb->u.cfg[8] = 0;
	init_config_cb->u.cfg[9] = 0; 
	init_config_cb->u.cfg[10] = 
		1 << 5 | 
		1 << 2 |
		1 << 1;
	init_config_cb->u.cfg[11] = 0;
	init_config_cb->u.cfg[12] = 6 << 4;
	init_config_cb->u.cfg[13] = 0x00;
	init_config_cb->u.cfg[14] = 0xF2;
	init_config_cb->u.cfg[15] = 
		1 << 7 | 
		1 << 6 | 
		1 << 3 | 
		1; 


	// Individual Address Configuration
	struct cb* ia_cb = get_next_cb();
	ia_cb->command = cb_ia_cmd | cb_el;
	__memcpy(ia_cb->u.mac_addr, _nic.mac, MAC_BYTE_LEN);
	__cio_printf("NIC: Setting IA to: ");
	mac_addr_print(ia_cb->u.mac_addr);
	__cio_printf("\n");

	// [init_config_cb]-->[ia_cb]-->
	set_mem32(&_nic.csr->scb.gen_ptr, (uint32_t) init_config_cb);
	set_mem8(&_nic.csr->scb.command, cuc_start);
	nic_write(&_nic);

	// Set IP address
	set_ip(0x6E6E325A); // set IP as 110.110.50.90
}

void nic_rx_enable() {
	set_mem32(&_nic.csr->scb.gen_ptr, (uint32_t) _nic.rfa.next);
	set_mem8(&_nic.csr->scb.command, ruc_start);
}

static uint32_t get_mem32(void* addr) {
	return *(volatile uint32_t*) addr;
}

static uint16_t get_mem16(void* addr) {
	return *(volatile uint16_t*) addr;
}

static uint8_t get_mem8(void* addr) {
	return *(volatile uint8_t*) addr;
}

static void set_mem32(void* addr, uint32_t value) {
	*(volatile uint32_t*) addr = value;
}

static void set_mem16(void* addr, uint16_t value) {
	*(volatile uint16_t*) addr = value;
}

static void set_mem8(void* addr, uint8_t value) {
	*(volatile uint8_t*) addr = value;
}
