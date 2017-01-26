#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "stdint.h"
#include "string.h"
#include "pc_support.h"
#include "xen_support.h"

#define __XEN_INTERFACE_VERSION__ 0x00040600
#include "public/xen.h"
#include "public/grant_table.h"
#include "public/event_channel.h"
#include "public/memory.h"
#include "public/hvm/hvm_op.h"
#include "public/hvm/params.h"
#include "public/io/xs_wire.h"
#include "public/io/ring.h"

static int store_evtchn;
static struct xenstore_domain_interface *xs_intf;
char dummy[XENSTORE_RING_SIZE];

#define NOTIFY() \
    do {\
        struct evtchn_send event;\
        event.port = store_evtchn;\
        hypercall(__HYPERVISOR_event_channel_op, EVTCHNOP_send, (uint32_t)&event, 0, 0, 0, 0);\
    } while(0)

#define IGNORE(n) \
    do {\
        readRsp(dummy, (n));\
    } while(0)

static void writeReq(char *msg, int len)
{
    int i;
    for (i = xs_intf->req_prod; len > 0; i++, len--)
    {
        XENSTORE_RING_IDX data;
        do {
            data = i - xs_intf->req_cons;
        } while (data >= XENSTORE_RING_SIZE);
        int ring_idx = MASK_XENSTORE_IDX(i);
        xs_intf->req[ring_idx] = *msg;
        msg++;
    }
    xs_intf->req_prod = i;
    return;
}

static void readRsp(char *buf, int len)
{
    int i;
    for (i = xs_intf->rsp_cons; len > 0; i++, len--)
    {
        XENSTORE_RING_IDX data;
        do {
            data = xs_intf->rsp_prod - i;
        } while (data == 0);
        int ring_idx = MASK_XENSTORE_IDX(i);
        *buf = xs_intf->rsp[ring_idx];
        buf++;
    }
    xs_intf->rsp_cons = i;
    return;
}

static int writeKeyValue(char *key, char *value)
{
    int k_len = strlen(key) + 1, v_len = strlen(value);
    struct xsd_sockmsg xs_header;
    xs_header.type = XS_WRITE;
    xs_header.req_id = 408;
    xs_header.tx_id = 0;
    xs_header.len = k_len + v_len;

    writeReq((char*)&xs_header, sizeof(struct xsd_sockmsg));
    writeReq(key, k_len);
    writeReq(value, v_len);

    NOTIFY();
    readRsp((char*)&xs_header, sizeof(struct xsd_sockmsg));
    if (xs_header.type == XS_ERROR) {
        readRsp(dummy, xs_header.len);
        printf("Error writing (%s, %s), %s.\n", key, value, dummy);
        return -1;
    }
    IGNORE(xs_header.len);
    return 0;
}

static int readValue(char *key, char *value, uint32_t value_len)
{
    int k_len = strlen(key) + 1;
    struct xsd_sockmsg xs_header;
    xs_header.type = XS_READ;
    xs_header.req_id = 804;
    xs_header.tx_id = 0;
    xs_header.len = k_len;
    writeReq((char*)&xs_header, sizeof(struct xsd_sockmsg));
    writeReq(key, k_len);
    NOTIFY();
    
    readRsp((char*)&xs_header, sizeof(struct xsd_sockmsg));
    if (xs_header.req_id != 804) {
        printf("Wrong req id (%d).\n", xs_header.req_id);
        IGNORE(xs_header.len);
        return -1;
    }

    if (value_len >= xs_header.len) {
        readRsp(value, xs_header.len);
        return 0;
    }

    readRsp(value, value_len);
    IGNORE(xs_header.len - value_len);
    return -2;
}

#define GET_EVTCHN_BIT(map, bit) \
    (((map)[(bit) / sizeof(xen_ulong_t)] & (1 << ((bit) % sizeof(xen_ulong_t)))) > 0)

#define SET_EVTCHN_BIT(map, bit) \
    (map)[(bit) / sizeof(xen_ulong_t)] |= (1 << ((bit) % sizeof(xen_ulong_t)))

#define CLEAR_EVTCHN_BIT(map, bit) \
    (map)[(bit) / sizeof(xen_ulong_t)] &= (~(1 << ((bit) % sizeof(xen_ulong_t))))

static void prvLoopTask( void *pvParameters )
{
    uint32_t base;
    base = xen_cpuid_base();
    if (base == 0) {
        printf("Xen hypervisor not found.\n\n");
        goto loop;
    }
    xen_init_hypercall_page(base);

    struct xen_hvm_param xhv;
    xhv.domid = DOMID_SELF;
    xhv.index = HVM_PARAM_STORE_EVTCHN;
    if (hypercall(__HYPERVISOR_hvm_op, HVMOP_get_param,
            (uint32_t)&xhv, 0, 0 ,0, 0) != 0) {
        printf("Failed to get event channel.\n");
        goto loop;
    }
    store_evtchn = xhv.value;
    xhv.index = HVM_PARAM_STORE_PFN;
    if (hypercall(__HYPERVISOR_hvm_op, HVMOP_get_param,
            (uint32_t)&xhv, 0, 0 ,0, 0) != 0) {
        printf("Failed to get machine frame number.\n");
        goto loop;
    }
    xs_intf = (struct xenstore_domain_interface *)((uint32_t)xhv.value << 12);

    struct shared_info *sip = (struct shared_info *)shared_info_page;
    struct xen_add_to_physmap xatp;
    xatp.domid = DOMID_SELF;
    xatp.idx = 0;
    xatp.space = XENMAPSPACE_shared_info;
    xatp.gpfn = (((uint32_t)sip) >> 12);
    if (hypercall(__HYPERVISOR_memory_op, XENMEM_add_to_physmap,
            (uint32_t)&xatp, 0, 0, 0, 0) != 0) {
        printf("Failed to map shared info page\n");
        goto loop;
    }

    evtchn_status_t chn_status;
    chn_status.dom = DOMID_SELF;
    chn_status.port = store_evtchn;
    hypercall(__HYPERVISOR_event_channel_op, EVTCHNOP_status,
        (uint32_t)&chn_status, 0, 0, 0, 0);
    if (chn_status.status == EVTCHNSTAT_interdomain) {
        printf("chn_status.status = EVTCHNSTAT_interdomain\n");
    } else {
        printf("chn_status.status = %x\n", chn_status.status);
    }

    printf("xenstore-write...\n");
    writeKeyValue("data", "408");
    CLEAR_EVTCHN_BIT(sip->evtchn_pending, store_evtchn);
    printf("Done.\n");

    char ret_value[128] = {0};
    int ret = readValue("data", ret_value, 128);
    printf("ret : %d\n", ret);
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
