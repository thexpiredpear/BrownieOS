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
#include <core/syscall.h>
#include <core/acpi.h>
#include <core/madt.h>
#include <core/apic.h>
#include <core/ioapic.h>
#include <mm/kmm.h>
#include <mm/paging.h>
#include <mm/vmm.h>
#include <drivers/hpet.h>
#include <proc/proc.h>
#include <proc/scheduler.h>

#define USER_TEST_CODE_VA   0x00400000
#define USER_TEST_DATA_VA   0x00401000
#define USER_TEST_STACK_TOP 0x00403000

extern page_directory_t* kernel_directory;

enum { USER_TEST_MESSAGE_COUNT = 2 };
static const char* const user_test_messages[USER_TEST_MESSAGE_COUNT] = {
    "[1] hello world from a process",
    "[2] hello world from another process",
};

static void kernel_single_process_test(void) {
    static proc_context_t user_ctx;
    const uint32_t user_stack_base = USER_TEST_STACK_TOP - PAGE_SIZE;

    uint32_t code_phys = alloc_pages(PMM_FLAGS_HIGHMEM, 1);
    uint32_t data_phys = alloc_pages(PMM_FLAGS_HIGHMEM, 1);
    uint32_t stack_phys = alloc_pages(PMM_FLAGS_HIGHMEM, 1);

    if(!code_phys || !data_phys || !stack_phys) {
        printf("iret test: failed to allocate user pages\n");
        return;
    }

    if(proc_map_pages(current_proc, USER_TEST_CODE_VA, code_phys, 1, true) != 0) {
        return;
    }

    if(proc_map_pages(current_proc, USER_TEST_DATA_VA, data_phys, 1, true) != 0) {
        return;
    }

    if(proc_map_pages(current_proc, user_stack_base, stack_phys, 1, true) != 0) {
        return;
    }

    flush_tlb();
	// fill page with NOP
    memset((void*)USER_TEST_CODE_VA, 0x90, PAGE_SIZE);
    uint8_t* code_ptr = (uint8_t*)USER_TEST_CODE_VA;
    size_t idx = 0;

    uint32_t data_offsets[USER_TEST_MESSAGE_COUNT];
    size_t message_lengths[USER_TEST_MESSAGE_COUNT];
    uint32_t data_cursor = 0;

    memset((void*)USER_TEST_DATA_VA, 0, PAGE_SIZE);
    for (size_t i = 0; i < USER_TEST_MESSAGE_COUNT; ++i) {
        const char* msg = user_test_messages[i];
        size_t len = strlen(msg);
        message_lengths[i] = len;
        data_offsets[i] = data_cursor;

        char* dest = (char*)(USER_TEST_DATA_VA + data_cursor);
        memcpy(dest, msg, len);
        dest[len] = '\0';

        data_cursor += (uint32_t)(len + 1);
    }

    for (size_t i = 0; i < USER_TEST_MESSAGE_COUNT; ++i) {
        uint32_t imm = SYS_PRINT_STRING;
        code_ptr[idx++] = 0xB8; // mov eax, imm32
        memcpy(&code_ptr[idx], &imm, sizeof(imm));
        idx += sizeof(imm);

        imm = USER_TEST_DATA_VA + data_offsets[i];
        code_ptr[idx++] = 0xBB; // mov ebx, imm32
        memcpy(&code_ptr[idx], &imm, sizeof(imm));
        idx += sizeof(imm);

        imm = (uint32_t)message_lengths[i];
        code_ptr[idx++] = 0xB9; // mov ecx, imm32
        memcpy(&code_ptr[idx], &imm, sizeof(imm));
        idx += sizeof(imm);

        code_ptr[idx++] = 0xCD;
        code_ptr[idx++] = 0x80; // int 0x80
    }

    code_ptr[idx++] = 0xEB;
    code_ptr[idx++] = 0xFE; // jmp $

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

    printf("Running user-mode syscall demo via iret trampoline...\n");
    iret_jump_user(&user_ctx);
    printf("syscall demo: returned unexpectedly from user mode\n");
}

// --- Three-process scheduling demo ---
// Creates three independent user processes. Each process maps its own code and
// data page at identical virtual addresses, prints a unique message via
// SYS_PRINT_STRING, then spins forever. The PIT preempts from user mode and
// the simple RR scheduler alternates execution between them.
static proc_t* make_user_proc_with_message(const char* msg) {
    if (!msg) return NULL;

    // Prepare user code/data pages (distinct physical frames per process)
    uint32_t code_phys = alloc_pages(PMM_FLAGS_HIGHMEM, 1);
    uint32_t data_phys = alloc_pages(PMM_FLAGS_HIGHMEM, 1);
    if (!code_phys || !data_phys) {
        printf("failed to allocate user pages\n");
        return NULL;
    }

    // Build the code and data via temporary kernel mappings
    uint8_t* code_ptr = (uint8_t*)kmap(code_phys);
    char*    data_ptr = (char*)kmap(data_phys);
    if (!code_ptr || !data_ptr) {
        printf("kmap failed for user pages\n");
        if (code_ptr) kunmap(code_ptr);
        if (data_ptr) kunmap(data_ptr);
        return NULL;
    }

    // Initialize pages
    memset(code_ptr, 0x90, PAGE_SIZE); // NOP pad
    memset(data_ptr, 0x00, PAGE_SIZE);

    // Stash message in data page at offset 0
    size_t msg_len = strlen(msg);
    memcpy(data_ptr, msg, msg_len);
    data_ptr[msg_len] = '\0';

    // Emit tiny user program:
    //   mov eax, SYS_PRINT_STRING
    //   mov ebx, USER_TEST_DATA_VA
    //   mov ecx, msg_len
    //   int 0x80
    //   jmp $
    size_t idx = 0;
    uint32_t imm;

    imm = SYS_PRINT_STRING;
    code_ptr[idx++] = 0xB8; // mov eax, imm32
    memcpy(&code_ptr[idx], &imm, sizeof(imm)); idx += sizeof(imm);

    imm = USER_TEST_DATA_VA; // message at offset 0
    code_ptr[idx++] = 0xBB; // mov ebx, imm32
    memcpy(&code_ptr[idx], &imm, sizeof(imm)); idx += sizeof(imm);

    imm = (uint32_t)msg_len;
    code_ptr[idx++] = 0xB9; // mov ecx, imm32
    memcpy(&code_ptr[idx], &imm, sizeof(imm)); idx += sizeof(imm);

    code_ptr[idx++] = 0xCD; // int 0x80
    code_ptr[idx++] = 0x80;

    code_ptr[idx++] = 0xEB; // jmp $
    code_ptr[idx++] = 0xFE;

    // Create process structure with entry at USER_TEST_CODE_VA and a 4 KiB stack
    proc_t* p = create_proc((void*)USER_TEST_CODE_VA, 0, PAGE_SIZE, 0, PROC_PRIORITY_NORMAL);
    if (!p) {
        printf("two-proc test: create_proc failed\n");
        kunmap(code_ptr);
        kunmap(data_ptr);
        return NULL;
    }

    // Map the prepared frames into the new process's address space
    if (proc_map_pages(p, USER_TEST_CODE_VA, code_phys, 1, true) != 0) {
        printf("two-proc test: map code failed\n");
        kunmap(code_ptr);
        kunmap(data_ptr);
        return NULL;
    }
    if (proc_map_pages(p, USER_TEST_DATA_VA, data_phys, 1, true) != 0) {
        printf("two-proc test: map data failed\n");
        kunmap(code_ptr);
        kunmap(data_ptr);
        return NULL;
    }

    // Drop temporary kernel mappings
    kunmap(code_ptr);
    kunmap(data_ptr);

    return p;
}

static void kernel_three_process_test(void) {
    // Messages mirror the original single-process demo
    const char* m1 = "[1] hello world from a process";
    const char* m2 = "[2] hello world from another process";
    const char* m3 = "[3] hello world from yet another process";

    proc_t* p1 = make_user_proc_with_message(m1);
    proc_t* p2 = make_user_proc_with_message(m2);
    proc_t* p3 = make_user_proc_with_message(m3);
    if (!p1 || !p2 || !p3) {
        printf("three-proc test: failed to set up processes\n");
        return;
    }

    printf("Launching three user processes; scheduler should alternate prints...\n");
    // Enter first process; PIT will preempt and round-robin to the second
    printf("Jumping to first process, entering user mode for the first time via iret\n");
    proc_enter(p1);
    // Not reached unless something unusual occurs
    printf("three-proc test: returned unexpectedly from user mode\n");
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
	syscall_init();
	init_acpi();
	parse_madt();
	//init_apic();
	pit_init(1000);
	//init_hpet(10000);
	
	// Original single-process demo:
	// kernel_process_test();
	kernel_three_process_test();
	kpause();
}
