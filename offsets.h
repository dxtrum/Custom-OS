/**
** @file offsets.h
**
** GENERATED AUTOMATICALLY - DO NOT EDIT
**
** This header file contains C Preprocessor macros which expand
** into the byte offsets needed to reach fields within structs
** used in the baseline system.  Should those struct declarations
** change, the Offsets program should be modified (if needed),
** recompiled, and re-run to recreate this file.
*/

#ifndef OFFSETS_H_
#define OFFSETS_H_

// Sizes of basic types

#define	SZ_char                	1
#define	SZ_short               	2
#define	SZ_int                 	4
#define	SZ_long                	4
#define	SZ_long_long           	8


// Sizes of our types

#define	SZ_int8_t              	1
#define	SZ_int16_t             	2
#define	SZ_int32_t             	4
#define	SZ_int64_t             	8
#define	SZ_uint8_t             	1
#define	SZ_uint16_t            	2
#define	SZ_uint32_t            	4
#define	SZ_uint64_t            	8
#define	SZ_uint_t              	4
#define	SZ_bool_t              	1
#define	SZ_pid_t               	4
#define	SZ_state_t             	1
#define	SZ_time_t              	4
#define	SZ_status_t            	4
#define	SZ_prio_t              	1
#define	SZ_queue_t             	4
#define	SZ_key_t               	4
#define	SZ_stack_t             	16384


// Offsets into context_t
// Size: 72 bytes

#define	CTX_ss                 	0
#define	CTX_gs                 	4
#define	CTX_fs                 	8
#define	CTX_es                 	12
#define	CTX_ds                 	16
#define	CTX_edi                	20
#define	CTX_esi                	24
#define	CTX_ebp                	28
#define	CTX_esp                	32
#define	CTX_ebx                	36
#define	CTX_edx                	40
#define	CTX_ecx                	44
#define	CTX_eax                	48
#define	CTX_vector             	52
#define	CTX_code               	56
#define	CTX_eip                	60
#define	CTX_cs                 	64
#define	CTX_eflags             	68


// Offsets into pcb_t
// Size: 32 bytes

#define	PCB_context            	0
#define	PCB_stack              	4
#define	PCB_wakeup             	8
#define	PCB_exit_status        	12
#define	PCB_pid                	16
#define	PCB_ppid               	20
#define	PCB_state              	24
#define	PCB_priority           	25
#define	PCB_quantum            	26
#define	PCB_ticks              	27
#define	PCB_filler             	28

#endif
