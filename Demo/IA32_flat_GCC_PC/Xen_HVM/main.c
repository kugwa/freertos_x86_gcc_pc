/*
    FreeRTOS V8.2.3 - Copyright (C) 2015 Real Time Engineers Ltd.
    All rights reserved

    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>>> AND MODIFIED BY <<<< the FreeRTOS exception.

    ***************************************************************************
    >>!   NOTE: The modification to the GPL is included to allow you to     !<<
    >>!   distribute a combined work that includes FreeRTOS without being   !<<
    >>!   obliged to provide the source code for proprietary components     !<<
    >>!   outside of the FreeRTOS kernel.                                   !<<
    ***************************************************************************

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available on the following
    link: http://www.freertos.org/a00114.html

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS provides completely free yet professionally developed,    *
     *    robust, strictly quality controlled, supported, and cross          *
     *    platform software that is more than just the market leader, it     *
     *    is the industry's de facto standard.                               *
     *                                                                       *
     *    Help yourself get started quickly while simultaneously helping     *
     *    to support the FreeRTOS project by purchasing a FreeRTOS           *
     *    tutorial book, reference manual, or both:                          *
     *    http://www.FreeRTOS.org/Documentation                              *
     *                                                                       *
    ***************************************************************************

    http://www.FreeRTOS.org/FAQHelp.html - Having a problem?  Start by reading
    the FAQ page "My application does not run, what could be wrong?".  Have you
    defined configASSERT()?

    http://www.FreeRTOS.org/support - In return for receiving this top quality
    embedded software for free we request you assist our global community by
    participating in the support forum.

    http://www.FreeRTOS.org/training - Investing in training allows your team to
    be as productive as possible as early as possible.  Now you can receive
    FreeRTOS training directly from Richard Barry, CEO of Real Time Engineers
    Ltd, and the world's leading authority on the world's leading RTOS.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, a DOS
    compatible FAT file system, and our tiny thread aware UDP/IP stack.

    http://www.FreeRTOS.org/labs - Where new FreeRTOS products go to incubate.
    Come and try FreeRTOS+TCP, our new open source TCP/IP stack for FreeRTOS.

    http://www.OpenRTOS.com - Real Time Engineers ltd. license FreeRTOS to High
    Integrity Systems ltd. to sell under the OpenRTOS brand.  Low cost OpenRTOS
    licenses offer ticketed support, indemnification and commercial middleware.

    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.

    1 tab == 4 spaces!
*/

/******************************************************************************
 * This project provides two demo applications.  A simple blinky style project,
 * and a more comprehensive test and demo application.  The
 * mainCREATE_SIMPLE_BLINKY_DEMO_ONLY setting (defined in this file) is used to
 * select between the two.  The simply blinky demo is implemented and described
 * in main_blinky.c.  The more comprehensive test and demo application is
 * implemented and described in main_full.c.
 *
 * This file implements the code that is not demo specific, including the
 * hardware setup and FreeRTOS hook functions.
 *
 * ENSURE TO READ THE DOCUMENTATION PAGE FOR THIS PORT AND DEMO APPLICATION ON
 * THE http://www.FreeRTOS.org WEB SITE FOR FULL INFORMATION ON USING THIS DEMO
 * APPLICATION, AND ITS ASSOCIATE FreeRTOS ARCHITECTURE PORT!
 * http://www.FreeRTOS.org/RTOS_Intel_Quark_Galileo_GCC.html
 *
 */

/* Standard includes. */
#include <stdlib.h>

/* Scheduler include files. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* Standard demo includes, only necessary for the tick hook. */
#include "TimerDemo.h"
#include "QueueOverwrite.h"
#include "EventGroupsDemo.h"
#include "QueueSet.h"
#include "TaskNotify.h"
#include "IntQueue.h"

/* 8295 and printf. */
#include "pc_support.h"

/* Set to 1 to sit in a loop on start up, allowing a debugger to connect to the
application before main() executes. */
#define mainWAIT_FOR_DEBUG_CONNECTION 		0

/* Set mainCREATE_SIMPLE_BLINKY_DEMO_ONLY to one to run the simple blinky demo,
or 0 to run the more comprehensive test and demo application. */
#define mainCREATE_SIMPLE_BLINKY_DEMO_ONLY	1

/*-----------------------------------------------------------*/

extern void main_tasks( void );

/* Prototypes for functions called from asm start up code. */
int main( void );
void CRT_Init( void );

/*
 * Prototypes for the standard FreeRTOS callback/hook functions implemented
 * within this file.
 */
void vApplicationMallocFailedHook( void );
void vApplicationIdleHook( void );
void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName );
void vApplicationTickHook( void );

/*
 * Perform any hardware/peripheral related initialisation necessary to run the
 * demo.
 */
static void prvSetupHardware( void );

/*
 * If mainWAIT_FOR_DEBUG_CONNECTION is set to 1 then the following function will
 * sit in a loop on start up, allowing a debugger to connect to the application
 * before main() executes.  If mainWAIT_FOR_DEBUG_CONNECTION is not set to 1
 * then the following function does nothing.
 */
static void prvLoopToWaitForDebugConnection( void );

/*-----------------------------------------------------------*/

/* See http://www.FreeRTOS.org/RTOS_Intel_Quark_Galileo_GCC.html for usage
instructions. */
int main( void )
{
	/* Optionally wait for a debugger to connect. */
	prvLoopToWaitForDebugConnection();

	/* Init the UART, GPIO, etc. */
	prvSetupHardware();

	main_tasks();

	return 0;
}
/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook( void )
{
	/* Called if a call to pvPortMalloc() fails because there is insufficient
	free memory available in the FreeRTOS heap.  pvPortMalloc() is called
	internally by FreeRTOS API functions that create tasks, queues, software
	timers, and semaphores.  The size of the FreeRTOS heap is set by the
	configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h.

	Force an assert. */
	configASSERT( xTaskGetTickCount() == 0 );
	taskDISABLE_INTERRUPTS();
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
	( void ) pcTaskName;
	( void ) pxTask;

	/* Run time stack overflow checking is performed if
	configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	function is called if a stack overflow is detected.

	Increase the size of the stack allocated to the offending task.

	Force an assert. */
	configASSERT( pxTask == NULL );
	taskDISABLE_INTERRUPTS();
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
	volatile unsigned long xFreeHeapSpace;

	/* This is just a trivial example of an idle hook.  It is called on each
	cycle of the idle task.  It must *NOT* attempt to block.  In this case the
	idle task just queries the amount of FreeRTOS heap that remains.  See the
	memory management section on the http://www.FreeRTOS.org web site for memory
	management options.  If there is a lot of heap memory free then the
	configTOTAL_HEAP_SIZE value in FreeRTOSConfig.h can be reduced to free up
	RAM. */
	xFreeHeapSpace = xPortGetFreeHeapSize();

	/* Remove compiler warning about xFreeHeapSpace being set but never used. */
	( void ) xFreeHeapSpace;
}
/*-----------------------------------------------------------*/

void vAssertCalled( const char * pcFile, unsigned long ulLine )
{
	printf( "ASSERT: File = %s, Line = %d\n", pcFile, ulLine );
}
/*-----------------------------------------------------------*/

void vApplicationTickHook( void )
{
	#if( mainCREATE_SIMPLE_BLINKY_DEMO_ONLY == 0 )
	{
		extern void vTimerPeriodicISRTests( void );

		/* The full demo includes a software timer demo/test that requires
		prodding periodically from the tick interrupt. */
		vTimerPeriodicISRTests();

		/* Call the periodic queue overwrite from ISR demo. */
		vQueueOverwritePeriodicISRDemo();

		/* Call the periodic event group from ISR demo. */
		vPeriodicEventGroupsProcessing();

		/* Call the periodic queue set from ISR demo. */
		vQueueSetAccessQueueSetFromISR();

		/* Use task notifications from an interrupt. */
		xNotifyTaskFromISR();
	}
	#endif
}
/*-----------------------------------------------------------*/

static void prvSetupHardware( void )
{
	/* Initialise HPET interrupt(s) */
	#if( ( mainCREATE_SIMPLE_BLINKY_DEMO_ONLY != 1 ) && ( hpetHPET_TIMER_IN_USE != 0 ) )
	{
		portDISABLE_INTERRUPTS();
		vInitializeAllHPETInterrupts();
	}
	#endif

	vScreenClear();
	vInitialize8259Chips();
}
/*-----------------------------------------------------------*/

static void prvLoopToWaitForDebugConnection( void )
{
	/* Debug if define = 1. */
	#if( mainWAIT_FOR_DEBUG_CONNECTION == 1 )
	{
		int i;
		for (i = 0; i < 1000000000; i++) ;
	}
	#endif
}
/*-----------------------------------------------------------*/

void CRT_Init( void )
{
extern uint32_t __bss_start[];
extern uint32_t __bss_end[];
extern uint32_t __data_vma[];
extern uint32_t __data_lma[];
extern uint32_t __data_start[];
extern uint32_t __data_end[];
uint32_t x = 255;
size_t xSize;

	/* Zero out bss. */
	xSize = ( ( size_t ) __bss_end ) - ( ( size_t ) __bss_start );
	memset( ( void * ) __bss_start, 0x00, xSize );

	/* Copy initialised variables. */
	xSize = ( ( size_t ) __data_end ) - ( ( size_t ) __data_start );
	memcpy( ( void * ) __data_vma, __data_lma, xSize );

	/* Ensure no interrupts are pending. */
	do
	{
		portAPIC_EOI = 0;
		x--;
	} while( x > 0 );
}
