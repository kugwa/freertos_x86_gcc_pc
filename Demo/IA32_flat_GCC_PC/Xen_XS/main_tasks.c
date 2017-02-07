#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "string.h"
#include "stdint.h"
#include "pc_support.h"
#include "xen_support.h"

static void prvLoopTask( void *pvParameters )
{
    if (xen_support_init() != 0) goto loop;

    /* Print shared memory */
    vTaskDelay(pdMS_TO_TICKS(5000));
    printf("%s\n", shm);

	/* Overwrite shared memory */
    memcpy(shm, "Bob", 4);

loop:
    (void)pvParameters;
    TickType_t xNextWakeTime = xTaskGetTickCount();
    while (1) vTaskDelayUntil(&xNextWakeTime, pdMS_TO_TICKS(1000));
}

#define mainLOOP_TASK_PRIORITY ( tskIDLE_PRIORITY + 1 )

void main_tasks( void )
{
    xTaskCreate(prvLoopTask, "Loop", configMINIMAL_STACK_SIZE * 2, NULL, \
                mainLOOP_TASK_PRIORITY, NULL);
    vTaskStartScheduler();
    for( ;; );
}
