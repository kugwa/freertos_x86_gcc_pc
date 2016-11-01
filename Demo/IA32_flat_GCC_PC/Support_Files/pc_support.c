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

/*-----------------------------------------------------------------------
 * Any required includes
 *------------------------------------------------------------------------
 */
#include "pc_support.h"

/*-----------------------------------------------------------------------
 * Any required local definitions
 *------------------------------------------------------------------------
 */
#ifndef NULL
	#define NULL (void *)0
#endif

#define MUTEX_WAIT_TIME	(( TickType_t ) 8 )

#define PIT_SECOND_DIV      (19)
#define PIT_RELOAD          (1193182 / PIT_SECOND_DIV)
#define PIT_RELOAD_LOW      (PIT_RELOAD & 0xff)
#define PIT_RELOAD_HIGH     ((PIT_RELOAD >> 8) & 0xff)

/*-----------------------------------------------------------------------
 * Function prototypes
 *------------------------------------------------------------------------
 */
extern void *memcpy( void *pvDest, const void *pvSource, unsigned long ulBytes );

/*-----------------------------------------------------------------------
 * Global variables
 *------------------------------------------------------------------------
 */
uint32_t bootinfo = 1UL;
uint32_t bootsign = 1UL;

/*-----------------------------------------------------------------------
 * Static variables
 *------------------------------------------------------------------------
 */
static uint16_t usIRQMask = 0xfffb;
static uint32_t pos = 0;
static uint32_t apic_count_per_ms;

/*------------------------------------------------------------------------
 * GDT default entries (used in GDT setup code)
 *------------------------------------------------------------------------
 */
static struct sd gdt_default[NGDE] =
{
	/*   sd_lolimit  sd_lobase   sd_midbase  sd_access   sd_hilim_fl sd_hibase */
	/* 0th entry NULL */
	{            0,          0,           0,         0,            0,        0, },
	/* 1st, Kernel Code Segment */
	{       0xffff,          0,           0,      0x9a,         0xcf,        0, },
	/* 2nd, Kernel Data Segment */
	{       0xffff,          0,           0,      0x92,         0xcf,        0, },
	/* 3rd, Kernel Stack Segment */
	{       0xffff,          0,           0,      0x92,         0xcf,        0, },
	/* 4st, Boot Code Segment */
	{       0xffff,          0,           0,      0x9a,         0xcf,        0, },
	/* 5th, Code Segment for BIOS32 request */
	{       0xffff,          0,           0,      0x9a,         0xcf,        0, },
	/* 6th, Data Segment for BIOS32 request */
	{       0xffff,          0,           0,      0x92,         0xcf,        0, },
};

extern struct sd gdt[];	/* Global segment table (defined in startup.S) */

/*------------------------------------------------------------------------
 * Set segment registers (used in GDT setup code)
 *------------------------------------------------------------------------
 */
void setsegs()
{
	extern int	__text_end;
	struct sd	*psd;
	uint32_t	np, ds_end;

	ds_end = 0xffffffff/PAGE_SIZE; 		/* End page number */

	psd = &gdt_default[1];				/* Kernel code segment */
	np = ((int)&__text_end - 0 + PAGE_SIZE-1) / PAGE_SIZE;	/* Number of code pages */
	psd->sd_lolimit = np;
	psd->sd_hilim_fl = FLAGS_SETTINGS | ((np >> 16) & 0xff);

	psd = &gdt_default[2];				/* Kernel data segment */
	psd->sd_lolimit = ds_end;
	psd->sd_hilim_fl = FLAGS_SETTINGS | ((ds_end >> 16) & 0xff);

	psd = &gdt_default[3];				/* Kernel stack segment */
	psd->sd_lolimit = ds_end;
	psd->sd_hilim_fl = FLAGS_SETTINGS | ((ds_end >> 16) & 0xff);

	psd = &gdt_default[4];				/* Boot code segment */
	psd->sd_lolimit = ds_end;
	psd->sd_hilim_fl = FLAGS_SETTINGS | ((ds_end >> 16) & 0xff);

	memcpy(gdt, gdt_default, sizeof(gdt_default));
}
/*-----------------------------------------------------------*/

/*------------------------------------------------------------------------
 * 8259 PIC initialization and support code
 *------------------------------------------------------------------------
 */
 void vInitialize8259Chips(void)
 {
	/* Set interrupt mask */
	uint16_t IRQMask = 0xffff;
	outb(IMR1, (uint8_t) (IRQMask & 0xff));
	outb(IMR2, (uint8_t) ((IRQMask >> 8) & 0xff));

	/* Initialise the 8259A interrupt controllers */

	/* Master device */
	outb(ICU1, 0x11);       /* ICW1: icw4 needed            */
	outb(ICU1+1, 0x20);     /* ICW2: base ivec 32           */
	outb(ICU1+1, 0x4);      /* ICW3: cascade on irq2        */
	outb(ICU1+1, 0x1);      /* ICW4: buf. master, 808x mode */

	/* Slave device */
	outb(ICU2, 0x11);       /* ICW1: icw4 needed            */
	outb(ICU2+1, 0x28);     /* ICW2: base ivec 40           */
	outb(ICU2+1, 0x2);      /* ICW3: slave on irq2          */
	outb(ICU2+1, 0xb);      /* ICW4: buf. slave, 808x mode  */

	/* always read ISR */
	outb(ICU1, 0xb);        /* OCW3: set ISR on read        */
	outb(ICU2, 0xb);        /* OCW3: set ISR on read        */

	/* Set interrupt mask - leave bit 2 enabled for IC cascade */
	IRQMask = 0xfffb;
	outb(IMR1, (uint8_t) (IRQMask & 0xff));
	outb(IMR2, (uint8_t) ((IRQMask >> 8) & 0xff));
 }
 /*-----------------------------------------------------------*/

 void vClearIRQMask(uint8_t IRQNumber)
 {
	 if( ( IRQNumber > 31 ) && ( IRQNumber < 48 ) )
	 {
		usIRQMask &= ~( 1 << (IRQNumber - 32 ) );
		usIRQMask &= 0xfffb;  	// bit 2 is slave cascade
		usIRQMask |= 0x0200;	// bit 14 is reserved
		outb(IMR1, (uint8_t) (usIRQMask & 0xff));
		outb(IMR2, (uint8_t) ((usIRQMask >> 8) & 0xff));
	 }
 }
 /*-----------------------------------------------------------*/

 void vSetIRQMask(uint8_t IRQNumber)
 {
	 if( ( IRQNumber > 31 ) && ( IRQNumber < 48 ) )
	 {
		usIRQMask |= ( 1 << (IRQNumber - 32 ) );
		usIRQMask &= 0xfffb;  	// bit 2 is slave cascade
		usIRQMask |= 0x0200;	// bit 14 is reserved
		outb(IMR1, (uint8_t) (usIRQMask & 0xff));
		outb(IMR2, (uint8_t) ((usIRQMask >> 8) & 0xff));
	 }
 }
 /*-----------------------------------------------------------*/

 /*-----------------------------------------------------------------------
  * 82C54 PIT (programmable interval timer) initialization
  *------------------------------------------------------------------------
  */
 void vInitializePIT(void)
 {
	/* Set the hardware clock: timer 0, 16-bit counter, rate                */
	/* generator mode, and counter runs in binary		                    */
	outb(CLKCNTL, 0x34);

	/* Set the clock rate to 1.193 Mhz, this is 1 ms interrupt rate         */
	uint16_t intrate = 1193;
	/* Must write LSB first, then MSB                                       */
	outb(CLKBASE, (char) (intrate & 0xff));
	outb(CLKBASE, (char) ((intrate >> 8) & 0xff));
 }
 /*-----------------------------------------------------------*/

/*-----------------------------------------------------------------------
 * Screen functions
 *------------------------------------------------------------------------
 */
void vScreenClear()
{
	char *pv = (char*)0xb8000;
	unsigned int i;

	for (i = 0; i < 80 * 25 * 2; i += 2) {
		pv[i] = ' ';
		pv[i + 1] = 0x07;
	}
	pos = 0;
}
/*-----------------------------------------------------------*/

void vScreenPutchar(int c)
{
	char *pv = (char*)0xb8000;

	if (pos >= 80 * 25 * 2) vScreenClear();
	if (c == '\n') {
		pos = (pos / 160 + 1) * 160;
	}
	else {
		pv[pos] = c;
		pv[pos + 1] = 0x07;
		pos += 2;
	}
}
 
/*-----------------------------------------------------------------------
 * APIC timer calibration and polling
 *------------------------------------------------------------------------
 */
void vCalibrateTimer(void)
{
    uint32_t remain_count;

    // initialize LAPIC to a well known state
    portAPIC_LDR = 0xFFFFFFFF;
    portAPIC_LDR = ((portAPIC_LDR & 0x00FFFFFF) | 0x00000001);
    portAPIC_LVT_TIMER = portAPIC_DISABLE;
    portAPIC_LVT_PERF = portAPIC_NMI;
    portAPIC_LVT_LINT0 = portAPIC_DISABLE;
    portAPIC_LVT_LINT1 = portAPIC_DISABLE;
    portAPIC_TASK_PRIORITY = 0;

    // software enable LAPIC
    portAPIC_SPURIOUS_INT = portAPIC_SPURIOUS_INT_VECTOR | portAPIC_ENABLE_BIT;
    portAPIC_LVT_ERROR = portAPIC_LVT_ERROR_VECTOR;
    portAPIC_LVT_TIMER = portAPIC_TIMER_INT_VECTOR;
    portAPIC_TMRDIV = portAPIC_DIV_16;

    // set PIT reload value
    outb(0x43, 0x34);
    outb(0x40, PIT_RELOAD_LOW);
    outb(0x40, PIT_RELOAD_HIGH);

    // reset LAPIC timer
    portAPIC_TIMER_INITIAL_COUNT = 0xffffffff;
 
    // wait until PIT counter wraps
    while(1) {
        inb(0x40);
        if (inb(0x40) != PIT_RELOAD_HIGH) break;
    }
    while(1) {
        inb(0x40);
        if (inb(0x40) == PIT_RELOAD_HIGH) break;
    }

    // save LAPIC timer remain count
    remain_count = portAPIC_TIMER_CURRENT_COUNT;
    portAPIC_LVT_TIMER = portAPIC_DISABLE;
    apic_count_per_ms = ((0xffffffff - remain_count) * PIT_SECOND_DIV) / 1000;
}
/*-----------------------------------------------------------*/

void vStartTimer(void)
{
    portAPIC_LVT_TIMER = portAPIC_TIMER_PERIODIC | portAPIC_TIMER_INT_VECTOR;
    portAPIC_TMRDIV = portAPIC_DIV_16;
    portAPIC_TIMER_INITIAL_COUNT = apic_count_per_ms;
}
/*-----------------------------------------------------------*/

void vPollUsTime(uint32_t us)
{
    uint32_t curr, last, elapse, goal;

    last = portAPIC_TIMER_CURRENT_COUNT;
    elapse = 0;
    goal = apic_count_per_ms * us / 1000;

    while (elapse < goal) {
        curr = portAPIC_TIMER_CURRENT_COUNT;
        if (curr <= last) elapse += last - curr;
        else elapse += (last - 0) + (apic_count_per_ms - curr);
        last = curr;
    }
}
