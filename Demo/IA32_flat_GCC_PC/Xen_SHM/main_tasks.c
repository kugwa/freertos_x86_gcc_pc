#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "stdint.h"
#include "pc_support.h"
#include "xen_support.h"

#define __XEN_INTERFACE_VERSION__ 0x00040600
#include "public/xen.h"
#include "public/grant_table.h"

static void prvLoopTask( void *pvParameters )
{
    uint32_t base;
    struct gnttab_map_grant_ref map_op;
    int ret;

    base = xen_cpuid_base();
    if (base == 0) {
        printf("Xen hypervisor not found.\n\n");
        goto loop;
    }
    xen_init_hypercall_page(base);

    map_op.host_addr = (uint32_t)grant_page;
    map_op.flags = GNTMAP_host_map;
    map_op.dom = 9;
    map_op.ref = 34;
    ret = hypercall(__HYPERVISOR_grant_table_op, GNTTABOP_map_grant_ref, (uint32_t)(&map_op), 1, 0, 0, 0);
    if(ret != 0 || map_op.status != GNTST_okay) {
        printf("__HYPERVISOR_grant_table_op() = %d\n\n", ret);
        goto loop;
    }

    printf("puts(grant_page) = \"%s\"\n\n", &grant_page[28]);
    printf("strcpy(grant_page, \"Bob\")");
    grant_page[28] = 'B';
    grant_page[29] = 'o';
    grant_page[30] = 'b';
    grant_page[31] = '\0';

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
