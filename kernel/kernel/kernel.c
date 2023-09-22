#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <kernel/common.h>
#include <kernel/multiboot.h>
#include <kernel/tty.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/pit.h>
#include <kernel/isr.h>
#include <kernel/mm/kheap.h>
#include <kernel/mm/paging.h>
#include <kernel/mm/vmm.h>

#ifndef KERNEL_VERSION
#define KERNEL_VERSION "0.0.1"
#endif
#ifndef KERNEL_ARCH
#define KERNEL_ARCH "x86"
#endif
	
void printlogo();

void kmain(multiboot_info_t* mbd, uint32_t magic) {
	terminal_initialize();
	gdt_init();
	idt_init();
	paging_init(mbd, magic);
	pit_init(100);
	char str[13];
	printf("BrownieOS version %s for %s\n\n", KERNEL_VERSION, KERNEL_ARCH);
	printlogo();
	int i = snprintf(str, 13, "Hello World!");
	printf("%s - Wrote %d characters\n", str, i);
	printf("Multiboot info at %x\n", mbd);
	printf("Multiboot magic number: %x\n", magic);
	kheap_init();
	uint32_t max_is_a_nerd_a = (uint32_t)kmalloc(0x100000);
	printf("kmalloc addr %x\n", max_is_a_nerd_a);
	uint32_t max_is_a_nerd_b = (uint32_t)kmalloc(1024);
	printf("kmalloc addr %x\n", max_is_a_nerd_b);
	// printsyms();
	// asm volatile("int $14");
	// 8, 10-14, 17, 21 
}

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
	extern uint32_t te;
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
	printf("%x TEXT END\n\n", te);
	printf("%x READONLY START\n", rs);
	printf("%x READONLY END\n\n", re);
	printf("%x DATA START\n", dtas);
	printf("%x DATA END\n\n", dtae);
	printf("%x BSS START\n", bs);
	printf("%x BSS END\n\n", be);
	printf("%x KERNEL END\n", ke);
}