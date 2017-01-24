#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "stdint.h"
#include "pc_support.h"
#include "xen_support.h"

#define __XEN_INTERFACE_VERSION__ 0x00040600
#include "public/xen.h"
#include "public/grant_table.h"
#include "public/hvm/hvm_op.h"
#include "public/hvm/params.h"
#include "public/io/xs_wire.h"
#include "public/event_channel.h"
static void prvLoopTask( void *pvParameters )
{
    uint32_t base;
    uint32_t version;
    gnttab_get_version_t gnttab_version;
    int ret;

    base = xen_cpuid_base();
    if (base == 0) {
        printf("Xen hypervisor not found.\n\n");
        goto loop;
    }

    xen_init_hypercall_page(base);
    printf("load hypercall_page.\n");

    //try to build xenstore communication 
    struct xen_hvm_param xhv;
    xhv.domid = DOMID_SELF;
    xhv.index = HVM_PARAM_STORE_EVTCHN;
    hypercall(__HYPERVISOR_hvm_op, HVMOP_get_param, &xhv, 0, 0 ,0, 0);
    int store_evtchn = xhv.value;
    xhv.index = HVM_PARAM_STORE_PFN;
    hypercall(__HYPERVISOR_hvm_op, HVMOP_get_param, &xhv, 0, 0 ,0, 0);
    unsigned int store_mfn = xhv.value;
    printf("store frame number : %x\n", store_mfn);
    struct xenstore_domain_interface *xs_intf = (store_mfn << 12);
    printf("interface->req : %x\n", xs_intf->req);
    printf("prod : %d, cons : %d\n", xs_intf->req_prod, xs_intf->req_cons);
    printf("interface->rsp : %x\n", xs_intf->rsp);
    printf("prod : %d, cons : %d\n", xs_intf->rsp_prod, xs_intf->rsp_cons);

    //prerpare socket header & body
    struct xsd_sockmsg xs_msg;
    xs_msg.type = XS_WRITE;
    xs_msg.req_id = 1;
    xs_msg.tx_id = 0;
    xs_msg.len = 7;
    char test_key[7] = "lab408\0", test_value[7] = "ggg\0"; //body
    
    //write to req ring buffer
    int i, length = 0;
    char *msg = (char*)&xs_msg;

    //send header
    for(i = xs_intf->req_prod; length < 16; i++, length++){
        XENSTORE_RING_IDX data;
        do{
            data = i - xs_intf->req_cons;
        }while(data >= XENSTORE_RING_SIZE);
        int ring_idx = MASK_XENSTORE_IDX(i);
        xs_intf->req[ring_idx] = msg[length];
    }
    xs_intf->req_prod = i;

    //send bopy
    length = 0;
    for(i = xs_intf->req_prod; length < 7; i++, length++){
        XENSTORE_RING_IDX data;
        do{
            data = i - xs_intf->req_cons;
        }while(data >= XENSTORE_RING_SIZE);
        int ring_idx = MASK_XENSTORE_IDX(i);
        xs_intf->req[ring_idx] = test_key[length];
    }
    length = 0;
    for(i = xs_intf->req_prod; length < 4; i++, length++){
        XENSTORE_RING_IDX data;
        do{
            data = i - xs_intf->req_cons;
        }while(data >= XENSTORE_RING_SIZE);
        int ring_idx = MASK_XENSTORE_IDX(i);
        xs_intf->req[ring_idx] = test_value[length];
    }
    xs_intf->req_prod = i;
    //notify domain 0
    struct evtchn_send event;
    event.port = store_evtchn;
    hypercall(__HYPERVISOR_event_channel_op, EVTCHNOP_send, &event, 0, 0, 0, 0);
    printf("this is the end of sending message.\n");
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
