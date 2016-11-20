#define PAGE_SIZE (1 << 12)

    .text
    .balign PAGE_SIZE
.globl  hypercall_page
hypercall_page:
    .skip   PAGE_SIZE

.globl  cpuid
cpuid:
    movl    4(%esp), %edi
    movl    (%edi), %eax
    cpuid
    movl    4(%esp), %edi
    movl    %eax, (%edi)
    movl    8(%esp), %edi
    movl    %ebx, (%edi)
    movl    12(%esp), %edi
    movl    %ecx, (%edi)
    movl    16(%esp), %edi
    movl    %edx, (%edi)
    ret

.globl  rdmsr
rdmsr:
    movl    4(%esp), %ecx
    rdmsr
    movl    8(%esp), %ecx
    movl    %eax, (%ecx)
    movl    12(%esp), %ecx
    movl    %edx, (%ecx)
    ret

.globl  wrmsr
wrmsr:
    movl    4(%esp), %ecx
    movl    8(%esp), %eax
    movl    12(%esp), %edx
    wrmsr
    ret

.globl  hypercall
hypercall:
    pushl   %ebp
    movl    8(%esp), %eax
    shll    $5, %eax
    addl    $hypercall_page, %eax
    movl    12(%esp), %ebx
    movl    16(%esp), %ecx
    movl    20(%esp), %edx
    movl    24(%esp), %esi
    movl    28(%esp), %edi
    movl    32(%esp), %ebp
    calll   %eax
    popl    %ebp
    ret

.end
