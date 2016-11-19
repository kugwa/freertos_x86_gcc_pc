#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "stdint.h"
#include "pc_support.h"
#include "xen_support.h"

#define mainLOOP_TASK_PRIORITY ( tskIDLE_PRIORITY + 1 )

static void prvLoopTask( void *pvParameters )
{
    (void)pvParameters;
    TickType_t xNextWakeTime;
    uint32_t base;
    uint32_t version;
    uint8_t *entry;
    int i;

    base = xen_cpuid_base();
    if (base == 0) {
        printf("Xen hypervisor not found.\n");
    } else {
        printf("Xen CPUID Base = 0x%x\n", base);
        version = xen_version(base);
        printf("Xen Version = %d.%d\n", version >> 16, version & 0xffff);
        xen_init_hypercall_page(base);
        printf("hypercall_page[0] = {");
        for (entry = (uint8_t*)hypercall_page, i = 0; i < 32; i++) {
            if (i % 8 == 0) printf("\n    ");
            printf("0x%x, ", entry[i]);
        }
        printf("\n}\n");
    }

    xNextWakeTime = xTaskGetTickCount();
    while (1) vTaskDelayUntil(&xNextWakeTime, pdMS_TO_TICKS(1000));
}

void main_tasks( void )
{
    xTaskCreate( prvLoopTask, "Loop", configMINIMAL_STACK_SIZE * 2, NULL, mainLOOP_TASK_PRIORITY, NULL );
    vTaskStartScheduler();
    for( ;; );
}
