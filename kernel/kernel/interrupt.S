.macro ISR_ERRCODE num
    .global isr\num
    isr\num :
        cli
        pushl $\num
        jmp isr_common_handler
.endm

.macro ISR_NOERRCODE num
    .global isr\num
    isr\num :
        cli
        pushl $0
        pushl $\num
        jmp isr_common_handler
.endm

.macro IRQ irqn num
    .global irq\irqn
    irq\irqn :
        cli
        pushl $0
        pushl $\num
        jmp irq_common_handler
.endm

.extern isr_handler

isr_common_handler:
    nop

    pushal

    pushw %ds
    pushw %es
    pushw %fs
    pushw %gs
    

    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    movl %esp, %eax
    pushl %eax

    # STACK TOP -> BOTTOM (low to high addresses)
    # PUSHED MANUALLY
    # uint16_t gs, uint16_t fs, uint16_t es, uint16_t ds
    # PUSHAD
    # uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp, uint32_t ebx, uint32_t edx, uint32_t ecx, uint32_t eax
    # PUSHED BY ISR 
    # uint32_t int_no, uint32_t err_code
    # PUSHED BY CPU
    # uint32_t eip, uint32_t cs, uint32_t eflags

    call isr_handler
    addl $0x4, %esp # remove pushed parameter

    # RESTORE REGISTERS IN STACK ORDER

    popw %gs
    popw %fs
    popw %es
    popw %ds

    popal

    addl $0x8, %esp # remove pushed err_code and int_no
    iret

.extern irq_handler

irq_common_handler:
    nop

    pushal

    pushw %ds
    pushw %es
    pushw %fs
    pushw %gs
    

    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    movl %esp, %eax
    pushl %eax

    # STACK TOP -> BOTTOM (low to high addresses)
    # PUSHED MANUALLY
    # uint16_t gs, uint16_t fs, uint16_t es, uint16_t ds
    # PUSHAD
    # uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp, uint32_t ebx, uint32_t edx, uint32_t ecx, uint32_t eax
    # PUSHED BY ISR 
    # uint32_t int_no, uint32_t err_code
    # PUSHED BY CPU
    # uint32_t eip, uint32_t cs, uint32_t eflags

    call irq_handler
    addl $0x4, %esp # remove pushed parameter

    # RESTORE REGISTERS IN STACK ORDER

    popw %gs
    popw %fs
    popw %es
    popw %ds

    popal

    addl $0x8, %esp # remove pushed err_code and int_no
    iret

ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE 8
ISR_NOERRCODE 9
ISR_ERRCODE 10
ISR_ERRCODE 11
ISR_ERRCODE 12
ISR_ERRCODE 13
ISR_ERRCODE 14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_ERRCODE 17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_ERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_NOERRCODE 30
ISR_NOERRCODE 31

IRQ 0 32
IRQ 1 33
IRQ 2 34
IRQ 3 35
IRQ 4 36
IRQ 5 37
IRQ 6 38
IRQ 7 39
IRQ 8 40
IRQ 9 41
IRQ 10 42
IRQ 11 43
IRQ 12 44
IRQ 13 45
IRQ 14 46
IRQ 15 47