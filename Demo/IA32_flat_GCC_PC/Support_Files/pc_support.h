/*--------------------------------------------------------------------
 Copyright(c) 2015 Intel Corporation. All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:

 * Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
 notice, this list of conditions and the following disclaimer in
 the documentation and/or other materials provided with the
 distribution.
 * Neither the name of Intel Corporation nor the names of its
 contributors may be used to endorse or promote products derived
 from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 --------------------------------------------------------------------*/

#ifndef __PC_SUPPORT_H__
#define __PC_SUPPORT_H__

#ifdef __cplusplus
	extern "C" {
#endif

//---------------------------------------------------------------------
// Any required includes
//---------------------------------------------------------------------
#include "FreeRTOS.h"
#include "semphr.h"
#include "pc_defs.h"
#include "HPET.h"

//---------------------------------------------------------------------
// Application main entry point
//---------------------------------------------------------------------
extern int main( void );

//---------------------------------------------------------------------
// Defines for GDT
//---------------------------------------------------------------------
#define	NGDE				8		/* Number of global descriptor entries	*/
#define FLAGS_GRANULARITY	0x80
#define FLAGS_SIZE			0x40
#define	FLAGS_SETTINGS		( FLAGS_GRANULARITY | FLAGS_SIZE )
#define	PAGE_SIZE			4096

struct __attribute__ ((__packed__)) sd
{
	unsigned short	sd_lolimit;
	unsigned short	sd_lobase;
	unsigned char	sd_midbase;
	unsigned char   sd_access;
	unsigned char	sd_hilim_fl;
	unsigned char	sd_hibase;
};

void setsegs();

//---------------------------------------------------------------------
// 8259 PIC (programmable interrupt controller) definitions
//---------------------------------------------------------------------
#define IMR1 (0x21)       /* Interrupt Mask Register #1           */
#define IMR2 (0xA1)       /* Interrupt Mask Register #2           */
#define ICU1 (0x20)
#define ICU2 (0xA0)
#define EOI  (0x20)

void vInitialize8259Chips(void);
void vClearIRQMask(uint8_t IRQNumber);
void vSetIRQMask(uint8_t IRQNumber);

//---------------------------------------------------------------------
// 82C54 PIT (programmable interval timer) definitions
//---------------------------------------------------------------------
#define GATE_CONTROL	0x61
#define CHANNEL2_DATA	0x42
#define	MODE_REGISTER	0x43
#define ONESHOT_MODE	0xB2
#define	CLKBASE			0x40
#define	CLKCNTL			MODE_REGISTER

void vInitializePIT(void);

//---------------------------------------------------------------------
// Screen functions
//---------------------------------------------------------------------
void vScreenClear(void);
void vScreenPutchar(int c);

//---------------------------------------------------------------------
// APIC timer definitions
//---------------------------------------------------------------------
#define APIC_IA32_APIC_BASE_MSR     0x1b
#define APIC_LOCAL_APIC_BASE        0xfee00000
#define APIC_LOCAL_APIC_REG(reg)    (*(uint32_t*)((APIC_LOCAL_APIC_BASE) + (reg)))
#define APIC_APICID                 0x20
#define APIC_APICVER                0x30
#define APIC_TASKPRIOR              0x80
#define APIC_EOI                    0xb0
#define APIC_LDR                    0xd0
#define APIC_DFR                    0xe0
#define APIC_SPURIOUS               0xf0
#define APIC_ESR                    0x280
#define APIC_ICRL                   0x300
#define APIC_ICRH                   0x310
#define APIC_LVT_TMR                0x320
#define APIC_LVT_PERF               0x340
#define APIC_LVT_LINT0              0x350
#define APIC_LVT_LINT1              0x360
#define APIC_LVT_ERR                0x370
#define APIC_TMRINITCNT             0x380
#define APIC_TMRCURRCNT             0x390
#define APIC_TMRDIV                 0x3e0
#define APIC_LAST                   0x38f
#define APIC_DISABLE                0x10000
#define APIC_SW_ENABLE              0x100
#define APIC_CPUFOCUS               0x200
#define APIC_NMI                    (4 << 8)
#define TMR_PERIODIC                0x20000
#define TMR_BASEDIV                 (1 << 20)

void vCalibrateTimer(uint32_t timer_vector, uint32_t error_vector, uint32_t spurious_vector);
void vPollUsTime(uint32_t us);

#ifdef __cplusplus
	} /* extern C */
#endif

#endif /* __PC_SUPPORT_H__ */
