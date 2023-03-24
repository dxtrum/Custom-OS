/*
** Author:    Donald Craig
** Professor: Prof. Carithers
** Class:     Systems Programming
** Date:      4/08/2022
** Read PCI bus configuration
*/

#ifndef _PCI_SCAN_H
#define _PCI_SCAN_H

#include "common.h"

// PCI Configuration standard offsets
#define PCI_VENDOR 0x00
#define PCI_DEVICE 0x02
#define PCI_COMMAND 0x04
#define PCI_STATUS 0x06
#define PCI_REV_ID 0x08
#define PCI_CLASS_CODE 0x09
#define PCI_CACHE_LINE_SIZE 0x0C
#define PCI_LATENCY_TIMER 0x0D
#define PCI_HEADER_TYPE 0x0E
#define PCI_BIST 0x0F
#define PCI_CSR_MEM_BASE_ADDR_REG 0x10
#define PCI_CSR_IO_BASE_ADDR_REG 0x14
#define PCI_FLASH_MEM_BASE_ADDR_REG 0x18
#define PCI_SUBSYSTEM_VENDOR_ID 0x2C
#define PCI_SUBSYSTEM_ID 0x2E
#define PCI_EXPANSION_ROM_BASE_ADDR_REG 0x30
#define PCI_CAP_PTR 0x34
#define PCI_INTERRUPT_LINE 0x3C
#define PCI_INTERRUPT_PIN 0x3D
#define PCI_MIN_GRANT 0x3E
#define PCI_MAX_LATENCY 0x3F
#define PCI_CAPABILITY_ID 0xDC
#define PCI_NEXT_ITEM_PTR 0xDD
#define PCI_POWER_MANAGEMENT_CAPABILITIES 0xDE
#define PCI_POWER_MANAGEMENT_CSR 0xE0
#define PCI_DATA 0xE2

/**
 * pci_cfg_read
 *
 * Read 32-bits at a time.
 * 
 * Reference: http://wiki.osdev.org/PCI#Configuration_Space
 * 
 */
uint32_t pci_cfg_read(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);

/**
 * pci_cfg_read_word
 *
 * Read 32-bits at a time. Use (offset & 0x02) = {0, 2}  
 * 
 * Reference: http://wiki.osdev.org/PCI#Configuration_Space
 * 
 */
uint16_t pci_cfg_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);

/**
 * pci_cfg_read_byte
 *
 * Read 32-bits at a time. Use (offset & 0x03) = {0, 1, 2, 3} 
 * 
 * Reference: http://wiki.osdev.org/PCI#Configuration_Space
 * 
 */
uint8_t pci_cfg_read_byte(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);

/**
 * pci_get_vendor
 *
 * Returns vendor 
 * Reference: http://wiki.osdev.org/PCI#Configuration_Space
 * 
 */
uint16_t pci_get_vendor(uint8_t bus, uint8_t slot);

/**
 * pci_get_device
 * 
 * Returns pci bus and slot for a specified vendor and device
 * @return 0 for success, -1 for failure
 * 
 */
int32_t pci_get_device(uint8_t* bus_ret, uint8_t* slot_ret, uint16_t vendor, uint16_t device);

/**
 * pci_dev_list
 * 
 * Prints out list of all devices found on the PCI bus
 */
void pci_dev_list(uint8_t max_bus, uint8_t max_slot);

#endif
