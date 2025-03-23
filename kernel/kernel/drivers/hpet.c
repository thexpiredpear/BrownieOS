#include <cpuid.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <core/isr.h>
#include <core/madt.h>
#include <core/apic.h>
#include <core/acpi.h>
#include <core/multiboot.h>
#include <core/common.h>
#include <mm/kmm.h>

volatile uint64_t* gen_cap = 0;
volatile uint64_t* gen_conf = 0;
volatile uint64_t* main_cnt = 0;
volatile uint64_t* tim_n_conf_base = 0;
volatile uint64_t* tim_n_comp_base = 0;
volatile uint32_t* gen_int_sts = 0;

uint32_t clk_per = 0;
uint32_t main_cnt_freq = 0;
uint32_t int_freq = 0;
uint32_t int_per = 0;

uint32_t ticks = 0;

void hpet_handler(int_regs_t* registers) {
    if(ticks % int_freq == 0) {
        printf("%u\n", ticks);
    }
    ticks++;
}

// TODO: clean up this entire function
void init_hpet(uint32_t freq) {
    hpet_t* hpet = get_hpet();
    if(hpet == NULL) {
        panic("hpet not found");
    }
    if(hpet->address_space_id != 0) {
        panic("UNSUPPORTED HPET ADDRESS SPACE ID");
    }

    void* hpet_addr = KP2V((hpet->addr & 0xFFFFFFFF));
    gen_cap = (volatile uint64_t*)hpet_addr;
    gen_conf = (volatile uint64_t*)(hpet_addr + 0x10);
    main_cnt = (volatile uint64_t*)(hpet_addr + 0xF0);
    tim_n_conf_base = (volatile uint64_t*)(hpet_addr + 0x100);
    tim_n_comp_base = (volatile uint64_t*)(hpet_addr + 0x108);
    gen_int_sts = (volatile uint32_t*)(hpet_addr + 0x20);
    clk_per = (uint32_t)((*gen_cap >> 32) & 0xFFFFFFFF);

    uint16_t ven_id = (uint16_t)((*gen_cap >> 16) & 0xFFFF);
    uint8_t leg_cap = (uint8_t)((*gen_cap >> 15) & 0x1);
    uint8_t cnt_cap = (uint8_t)((*gen_cap >> 13) & 0x1);
    uint8_t num_tim = (uint8_t)(((*gen_cap >> 8) & 0x1F) + 1);
    uint8_t rev_id = (uint8_t)(*gen_cap & 0xFF);
    *gen_conf &= ~((uint64_t)0b11);
    *main_cnt = 0;

    uint64_t tim_n_disable_mask = ~((uint64_t)(0b11111 << 9) | (1 << 2) | (1 << 3) | (1 << 6));
    uint64_t tim_n_enable_mask = (0b00010 << 9) | (1 << 2) | (1 << 3) | (1 << 6);

    for(uint8_t i = 0; i < num_tim; i++) {
        bool flag = false;
        volatile uint64_t* tim_n_conf = (volatile uint64_t*)((uint32_t)tim_n_conf_base + (0x20 * i));
        uint32_t cap_apic = (uint32_t)((*tim_n_conf >> 32) & 0xFFFFFFFF);
        uint8_t cap_size = (uint8_t)((*tim_n_conf >> 5) & 0x1);
        uint8_t cap_per = (uint8_t)((*tim_n_conf >> 4) & 0x1);
        if(cap_size != 1) {
            printf("TIMER NOT 64 BIT CAPABLE\n");
            flag = true;
        }
        if(cap_per != 1) {
            printf("TIMER NOT PERIODIC CAPABLE\n");
            flag = true;
        }
        if(((cap_apic >> 2) & 0x1) != 1) {
            printf("TIMER NOT IRQ2 CAPABLE\n");
            flag = true;
        }
        if(flag) {
            panic("TIMER SETUP FAILED");
        }
        // clear bits 2 3 8 9-13
        *tim_n_conf &= tim_n_disable_mask;    
    }

    isr_set_handler(32, &hpet_handler);
    int_freq = freq;
    main_cnt_freq = (uint64_t)((double)1000000000000000 / (double)clk_per);
    int_per = (uint32_t)((double)main_cnt_freq / (double)int_freq);
    *tim_n_conf_base |= tim_n_enable_mask;
    *tim_n_comp_base = int_per;
    *gen_conf |= 0b1;
    uint64_t threshold = 0;  
}
