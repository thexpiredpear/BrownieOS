.global idt_flush
idt_flush:
    mov 4(%esp), %eax   # IDT pointer passed as parameter
    lidt (%eax)         # Load IDT pointer into IDTR
    ret