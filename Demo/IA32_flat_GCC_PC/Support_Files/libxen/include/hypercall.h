#include "stdint.h"

void cpuid(uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx);
void rdmsr(uint32_t msr, uint32_t *low, uint32_t *high);
void wrmsr(uint32_t msr, uint32_t low, uint32_t high);
uint32_t hypercall(uint32_t hypercall_num,
                   uint32_t arg1, uint32_t arg2, uint32_t arg3,
                   uint32_t arg4, uint32_t arg5, uint32_t arg6);
