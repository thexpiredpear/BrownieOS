#include <cpuid.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <core/madt.h>
#include <core/apic.h>
#include <core/acpi.h>
#include <core/multiboot.h>
#include <core/common.h>
#include <mm/vmm.h>
#include <mm/kmm.h>

void init_hpet() {
    hpet_t* hpet = get_hpet();
    if(hpet == NULL) {
        panic("hpet not found");
    }
    if(hpet->address_space_id != 0) {
        panic("UNSUPPORTED HPET ADDRESS SPACE ID");
    }
    void* hpet_addr = access_paddr_DANGER((void*)(hpet->addr & 0xFFFFFFFF));
    volatile uint64_t* gen_cap = (volatile uint64_t*)hpet_addr;
    volatile uint64_t* gen_conf = (volatile uint64_t*)(hpet_addr + 0x10);
    volatile uint64_t* main_cnt = (volatile uint64_t*)(hpet_addr + 0xF0);
    volatile uint64_t* tim_n_conf_base = (volatile uint64_t*)(hpet_addr + 0x100);
    uint32_t clk_per = (uint32_t)((*gen_cap >> 32) & 0xFFFFFFFF);
    uint16_t ven_id = (uint16_t)((*gen_cap >> 16) & 0xFFFF);
    uint8_t leg_cap = (uint8_t)((*gen_cap >> 15) & 0x1);
    uint8_t cnt_cap = (uint8_t)((*gen_cap >> 13) & 0x1);
    uint8_t num_tim = (uint8_t)(((*gen_cap >> 8) & 0x1F) + 1);
    uint8_t rev_id = (uint8_t)(*gen_cap & 0xFF);
    printf("CLK_PER: 0x%x | VEN_ID: 0x%x | LEG_CAP: %i | CNT_CAP: %i | NUM_TIM: %i | REV_ID: %i\n\n", clk_per, ven_id, leg_cap, cnt_cap, num_tim, rev_id);
    *gen_conf &= ~((uint64_t)0b11);
    *main_cnt = 0;
    for(uint8_t i = 0; i < num_tim; i++) {
        printf("---- TIMER %i ----\n", i);
        volatile uint64_t* tim_n_conf = (volatile uint64_t*)((uint32_t)tim_n_conf_base + (0x20 * i));
        uint32_t cap_apic = (uint32_t)((*tim_n_conf >> 32) & 0xFFFFFFFF);
        uint8_t cur_apic = (uint8_t)((*tim_n_conf >> 9) & 0x1F);
        uint8_t cur_size = (uint8_t)((*tim_n_conf >> 8) & 0x1);
        uint8_t cap_size = (uint8_t)((*tim_n_conf >> 5) & 0x1);
        uint8_t cap_per = (uint8_t)((*tim_n_conf >> 4) & 0x1);
        for(int j = 0; j < 32; j++) {
            if((cap_apic >> j) & 0x1 == 1) {
                printf("IRQ%i CAPABLE\n", j);
            }
        }
        if(cap_size != 1) {
            panic("TIMER NOT 64 BIT CAPABLE");
        } else {
            printf("TIMER 64 BIT CAPABLE\n");
        }
        if(cap_per != 1) {
            panic("TIMER NOT PERIODIC CAPABLE");
        } else {
            printf("TIMER PERIODIC CAPABLE\n");
        }
        printf("CUR_APIC: IRQ%i | CUR_SIZE: %i\n\n", cur_apic, (cur_size == 1) ? 64 : 32);
    }
}
