.globl iret_to_user
iret_to_user:
    # args: (uint32_t eip, uint32_t esp, uint32_t eflags)
    # Load user data segments (0x23)
    mov $0x23, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    # Fetch arguments
    mov 4(%esp), %eax   # eip
    mov 8(%esp), %ecx   # esp
    mov 12(%esp), %edx  # eflags
    or $0x200, %edx     # ensure IF=1

    # Build iret frame for user mode
    push $0x23          # SS (user data)
    push %ecx           # ESP
    push %edx           # EFLAGS
    push $0x1B          # CS (user code)
    push %eax           # EIP
    iret

