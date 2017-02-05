#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "stdint.h"
#include "pc_support.h"
#include "xen_support.h"
#include "xenstore.h"

static void prvLoopTask( void *pvParameters )
{
    uint32_t base;
    base = xen_cpuid_base();
    if (base == 0) {
        printf("Xen hypervisor not found.\n\n");
        goto loop;
    }
    xen_init_hypercall_page(base);

    xenstore_init();

    printf("xenstore-write...\n");
    xenstore_write("data", "408");
    printf("Done.\n");

    char ret_value[128] = {0};
    xenstore_read("data", ret_value, 128);
    printf("readValue(\"data\") = %s\n", ret_value);

loop:
    (void)pvParameters;
    TickType_t xNextWakeTime = xTaskGetTickCount();
    while (1) vTaskDelayUntil(&xNextWakeTime, pdMS_TO_TICKS(1000));
}

#define mainLOOP_TASK_PRIORITY ( tskIDLE_PRIORITY + 1 )

void main_tasks( void )
{
    xTaskCreate( prvLoopTask, "Loop", configMINIMAL_STACK_SIZE * 2, NULL, mainLOOP_TASK_PRIORITY, NULL );
    vTaskStartScheduler();
    for( ;; );
}
