#ifndef _KERNEL_VMM_H
#define _KERNEL_VMM_H 1

#include <stdint.h>
#include <mm/paging.h>

// Maps a 4 KiB-aligned PHYSICAL address `paddr` into the kernel’s virtual
// address space. If `paddr` lies within the pre-mapped lowmem window, returns
// `KP2V(paddr)`; otherwise finds a free PTE in the kernel HIGHMEM window and
// installs a temporary mapping. Returns a KERNEL virtual address with the
// original offset preserved for sub-page addresses.
void* kmap(uint32_t paddr);
// Unmaps a kernel virtual address previously returned by `kmap`. No-op for
// NULL or addresses outside the kernel window. Clears the corresponding PTE in
// the HIGHMEM window; does not free the underlying physical frame or flush TLB.
void kunmap(void* vaddr);

#endif
