#include "stdint.h"
#include "xen_support.h"

uint32_t xen_cpuid_base()
{
    uint32_t base, eax, ebx, ecx, edx;

    for (base = 0x40000000; base < 0x40010000; base += 0x100) {
        eax = base;
        cpuid(&eax, &ebx, &ecx, &edx);
        if (ebx == 0x566e6558 && ecx == 0x65584d4d && edx == 0x4d4d566e) {
            return base;
        } /* ebx == "XenV" && ecx == "MMXe" && edx == "nVMM" */
    }

    return 0;
}

uint32_t xen_version(uint32_t base)
{
    uint32_t eax, ebx, ecx, edx;

    eax = base + 1;
    cpuid(&eax, &ebx, &ecx, &edx);

    return eax;
}

void xen_init_hypercall_page(uint32_t base)
{
    uint32_t eax, ebx, ecx, edx;

    eax = base + 2;
    cpuid(&eax, &ebx, &ecx, &edx);
    wrmsr(ebx, hypercall_page, 0);
}
