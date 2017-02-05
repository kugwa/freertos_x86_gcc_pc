#ifndef __XEN_INTERFACE_VERSION__
#warning "Please specify xen interface version by defining __XEN_INTERFACE_VERSION__."
#endif

#include "stdint.h"
#include "string.h"
#include "pc_support.h"
#include "xen_support.h"

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
static char dummy[XENSTORE_RING_SIZE];

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

static void writeReq(const char *msg, int len)
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

int xenstore_write(const char *key, const char *value)
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

int xenstore_read(const char *key, char *value, uint32_t value_len)
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

int xenstore_init()
{
    struct xen_hvm_param xhv;
    xhv.domid = DOMID_SELF;
    xhv.index = HVM_PARAM_STORE_EVTCHN;
    if (hypercall(__HYPERVISOR_hvm_op, HVMOP_get_param,
            (uint32_t)&xhv, 0, 0 ,0, 0) != 0) {
        printf("Failed to get event channel.\n");
        return -1;
    }
    store_evtchn = xhv.value;
    xhv.index = HVM_PARAM_STORE_PFN;
    if (hypercall(__HYPERVISOR_hvm_op, HVMOP_get_param,
            (uint32_t)&xhv, 0, 0 ,0, 0) != 0) {
        printf("Failed to get machine frame number.\n");
        return -1;
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
        return -1;
    }

    return 0;
}
