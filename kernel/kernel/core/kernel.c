#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <core/common.h>
#include <core/multiboot.h>
#include <drivers/tty.h>
#include <core/gdt.h>
#include <core/idt.h>
#include <drivers/pit.h>
#include <core/isr.h>
#include <core/acpi.h>
#include <core/madt.h>
#include <core/apic.h>
#include <core/ioapic.h>
#include <mm/kmm.h>
#include <mm/paging.h>
#include <drivers/hpet.h>
#include <proc/proc.h>

#define USER_TEST_CODE_VA   0x00400000
#define USER_TEST_STACK_TOP 0x00402000

extern page_directory_t* kernel_directory;

static const uint8_t user_priv_test_code[] = {
    0xFA,       // cli (privileged; should trigger #GP in CPL3)
    0xEB, 0xFE  // jmp $ to avoid falling through if instruction ever succeeded
};

static int map_single_user_page(uint32_t virt, uint32_t phys, bool writable) {
    uint32_t dir_idx = PAGE_DIR_IDX(virt);
    uint32_t tbl_idx = PAGE_TBL_IDX(virt);

    page_dir_entry_t* entry = &kernel_directory->page_dir_entries[dir_idx];
    page_table_t* table = kernel_directory->tables[dir_idx];

    if(!entry->present) {
        uint32_t table_phys = alloc_pages(PMM_FLAGS_DEFAULT, 1);
        if(!table_phys) {
            printf("iret test: failed to allocate page table\n");
            return -1;
        }
        table = (page_table_t*)KP2V(table_phys);
        memset(table, 0, sizeof(page_table_t));

        kernel_directory->tables[dir_idx] = table;
        entry->present = 1;
        entry->rw = 1;
        entry->user = 1;
        entry->frame = PAGE_FRAME(table_phys);
    } else {
        if(!table) {
            printf("iret test: PDE present without table pointer\n");
            return -1;
        }
        entry->user = 1;
        entry->rw = 1;
    }

    set_page(&table->pages[tbl_idx], PAGE_FRAME(phys), true, writable, true);
    return 0;
}

static void run_iret_privilege_smoke(void) {
    static proc_context_t user_ctx;
    const uint32_t user_stack_base = USER_TEST_STACK_TOP - PAGE_SIZE;

    uint32_t code_phys = alloc_pages(PMM_FLAGS_HIGHMEM, 1);
    uint32_t stack_phys = alloc_pages(PMM_FLAGS_HIGHMEM, 1);

    if(!code_phys || !stack_phys) {
        printf("iret test: failed to allocate user pages\n");
        return;
    }

    if(map_single_user_page(USER_TEST_CODE_VA, code_phys, true) != 0) {
        return;
    }

    if(map_single_user_page(user_stack_base, stack_phys, true) != 0) {
        return;
    }

    flush_tlb();
	// fill page with NOP
    memset((void*)USER_TEST_CODE_VA, 0x90, PAGE_SIZE);
	// insert privileged instruction test code
    memcpy((void*)USER_TEST_CODE_VA, user_priv_test_code, sizeof(user_priv_test_code));
	// clear user stack
    memset((void*)user_stack_base, 0, PAGE_SIZE);

    memset(&user_ctx, 0, sizeof(user_ctx));
    user_ctx.eip = USER_TEST_CODE_VA;
    user_ctx.cs = 0x1B;
    user_ctx.eflags = 0x202;
    user_ctx.useresp = USER_TEST_STACK_TOP - 0x10;
    user_ctx.ss = 0x23;
    user_ctx.esp = user_ctx.useresp;
    user_ctx.ebp = user_ctx.useresp;

    printf("Running privileged instruction smoke-test via iret trampoline...\n");
    iret_jump_user(&user_ctx);
    printf("iret test: returned unexpectedly from user mode\n");
}

#ifndef KERNEL_VERSION
#define KERNEL_VERSION "0.0.1"
#endif
#ifndef KERNEL_ARCH
#define KERNEL_ARCH "x86"
#endif

void printlogo() {
	printf(R"(
,-----.                                   ,--.            ,-----.  ,---.   
|  |) /_ ,--.--. ,---. ,--.   ,--.,--,--, `--' ,---.     '  .-.  ''   .-'  
|  .-.  \|  .--'| .-. ||  |.'.|  ||      \,--.| .-. :    |  | |  |`.  `-.  
|  '--' /|  |   ' '-' '|   .'.   ||  ||  ||  |\   --.    '  '-'  '.-'    | 
`------' `--'    `---' '--'   '--'`--''--'`--' `----'     `-----' `-----')");
	printf("\n\n");
}

void printsyms() {
	extern uint32_t ks;
	extern uint32_t ke;
	extern uint32_t ts;

	extern uint32_t rs;
	extern uint32_t re;
	extern uint32_t dtas;
	extern uint32_t dtae;
	extern uint32_t bs;
	extern uint32_t be;
	extern uint32_t test;
	printf("%x KERNEL START\n\n", ks);
	printf("%x TEST \n\n", test);
	printf("%x TEXT START\n", ts);
	//printf("%x TEXT END\n\n", te);
	printf("%x READONLY START\n", rs);
	printf("%x READONLY END\n\n", re);
	printf("%x DATA START\n", dtas);
	printf("%x DATA END\n\n", dtae);
	printf("%x BSS START\n", bs);
	printf("%x BSS END\n\n", be);	
	printf("%x KERNEL END\n", ke);
}

void kpause() {
	while(1) {
		asm volatile("hlt");
	}
}

void kmain(multiboot_info_t* mbd, uint32_t magic) {
	gdt_init();
	idt_init();
	paging_init(mbd, magic);
	printf("BrownieOS kernel version %s for %s\n\n", KERNEL_VERSION, KERNEL_ARCH);
	printlogo();
	kheap_init();
	proc_init();
	kernel_proc_init();
	scheduler_init();
	isr_init();
	init_acpi();
	parse_madt();
	//init_apic();
	pit_init(1000);
	//init_hpet(10000);
	
	run_iret_privilege_smoke();
	kpause();
}
