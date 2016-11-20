#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "stdint.h"
#include "pc_support.h"
#include "xen_support.h"

#define __XEN_INTERFACE_VERSION__ 0x00040600
#include "xen.h"
#include "grant_table.h"

static void prvLoopTask( void *pvParameters )
{
    uint32_t base;
    uint32_t version;
    int i;
    gnttab_get_version_t gnttab_version;
    int ret;

    base = xen_cpuid_base();
    if (base == 0) {
        printf("Xen hypervisor not found.\n\n");
        goto loop;
    }
    version = xen_version(base);
    printf("Xen Version = %d.%d\n\n", version >> 16, version & 0xffff);

    xen_init_hypercall_page(base);
    printf("hypercall_page[__HYPERVISOR_grant_table_op] = {");
    for (i = 0; i < 32; i++) {
        if (i % 8 == 0) printf("\n    ");
        printf("0x%x, ", ((uint8_t*)(&hypercall_page[__HYPERVISOR_grant_table_op]))[i]);
    }
    printf("\n}\n\n");

    gnttab_version.dom = DOMID_SELF;
    gnttab_version.version = 0;
    ret = hypercall(__HYPERVISOR_grant_table_op, GNTTABOP_get_version,
                    (uint32_t)(&gnttab_version), 0, 0, 0, 0);
    if (ret != 0) {
        printf("hypercall(__HYPERVISOR_grant_table_op, GNTTABOP_get_version) = %d\n\n", ret);
        goto loop;
    }
    printf("Grant Table Version = %d\n\n", gnttab_version.version);

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
