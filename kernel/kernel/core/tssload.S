.global tss_flush

# Load Task Register with the provided selector (16-bit)
tss_flush:
    movw 4(%esp), %ax
    ltr %ax
    ret

