#define PAGE_SIZE (1 << 12)

    .data
    .balign PAGE_SIZE
.globl  grant_page
grant_page:
    .skip   PAGE_SIZE

    .balign PAGE_SIZE
.globl  shared_info_page
shared_info_page:
    .skip   PAGE_SIZE

    .text
    .balign PAGE_SIZE
.globl  hypercall_page
hypercall_page:
    .skip   PAGE_SIZE
