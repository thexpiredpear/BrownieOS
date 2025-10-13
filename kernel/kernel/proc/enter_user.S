.global iret_jump_user

# void iret_jump_user(proc_context_t* ctx)
# Expects: ctx at 4(%esp)
# Uses ctx fields to construct an iret frame for ring 3:
#   [SS, ESP, EFLAGS, CS, EIP], then executes iret.
# Layout of proc_context_t (see kernel/include/proc/proc.h):
#   0x20: eip, 0x24: cs, 0x28: eflags, 0x2C: useresp, 0x30: ss
iret_jump_user:
    mov 4(%esp), %eax        # ctx pointer

    # Optional: preload user data segment into DS/ES/FS/GS to avoid GP in userland
    movw 48(%eax), %cx       # ctx->ss (16-bit selector)
    movw %cx, %ds
    movw %cx, %es
    movw %cx, %fs
    movw %cx, %gs

    # Build iret frame for privilege change to ring 3
    pushl 48(%eax)           # SS
    pushl 44(%eax)           # ESP (useresp)
    pushl 40(%eax)           # EFLAGS
    pushl 36(%eax)           # CS
    pushl 32(%eax)           # EIP

    iret                     # Never returns

