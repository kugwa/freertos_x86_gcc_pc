#include "stdint.h"

uint32_t xen_cpuid_base(void);
uint32_t xen_version(uint32_t base);
void xen_init_hypercall_page(uint32_t base);
