#include "stdint.h"
#include "pc_support.h"
#include "xen_support.h"

int xen_support_init()
{
    uint32_t base;
    int ret;

    base = xen_cpuid_base();
    if (base == 0) {
        printf("%s: Xen hypervisor not found.\n", __func__);
        return -1;
    }
    xen_init_hypercall_page(base);

    ret = xenstore_init();
    if (ret != 0) {
        printf("%s: Failed to initialize Xenstore.\n", __func__);
        return -1;
    }

    return 0;
}
