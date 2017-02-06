#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "stdint.h"
#include "pc_support.h"
#include "xen_support.h"

#include "public/xen.h"
#include "public/grant_table.h"

static int atoi(char *s)
{
	int slen = 0;
	while( s[slen] != '\0' ) slen++;
	
	int res = 0;
	int temp = 1;
	int i = slen-1;
	for( ; i >= 0; i-- ){
		res += ((s[i] - 48)*temp) ;
		temp *= 10;
	}
		
	return res;
}

static void prvLoopTask( void *pvParameters )
{
    struct gnttab_map_grant_ref map_op;
    char ref[5], dom[5];
    int i, ret;

    if (xen_support_init() != 0) goto loop;

    /* Get domW's dom and ref */
    do {
        for (i = 0; i < 5; i++) ref[i] = 0;
        ret = xenstore_read("data/nex/remote-gref", ref, 3);
    } while( ret < 0 || ref[0] == 0 );
    do {
        for (i = 0; i < 5; i++) dom[i] = 0;
        ret = xenstore_read("data/nex/remote-dom", dom, 3);
    } while( ret < 0 || dom[0] == 0 );

    /* Map domW's page */
    map_op.host_addr = (uint32_t)grant_page;
    map_op.flags = GNTMAP_host_map;
    map_op.dom = atoi(dom);
    map_op.ref = atoi(ref);
    ret = hypercall(__HYPERVISOR_grant_table_op, GNTTABOP_map_grant_ref, \
                    (uint32_t)(&map_op), 1, 0, 0, 0);
    if(ret != 0 || map_op.status != GNTST_okay) {
        printf("%s: __HYPERVISOR_grant_table_op() = %d, " \
               "dom = %d, ref = %d\n", \
               __func__, ret, map_op.dom, map_op.ref);
        goto loop;
    }

    printf("%s: grant_page[] = \"%s\"\n", __func__, &grant_page[28]);
    printf("%s: strcpy(grant_page, \"Bob\")\n", __func__);
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
    xTaskCreate(prvLoopTask, "Loop", configMINIMAL_STACK_SIZE * 2, NULL, \
                mainLOOP_TASK_PRIORITY, NULL);
    vTaskStartScheduler();
    for( ;; );
}
