ENTRY (_start)

SECTIONS
{
    . = 0x00100000;
	/* The kernel will live at 3GB + 1MiB in the virtual address space, */
	/* which will be mapped to 1MiB in the physical address space. */
	/* Note that we page-align the sections. */

	_kernel_start = .;
        .multiboot.data : {
            *(.multiboot.data)
        }

       .multiboot.text : {
           *(.multiboot.text)
       }

	. += 0xC0000000;
	_test = .;
	/* Add a symbol that indicates the start address of the kernel. */
	.text ALIGN (4K) : AT (ADDR (.text) - 0xC0000000)
    {
        _text_start = .;
        *(.text)
        _text_end = .;
    }

    .rodata ALIGN (4K) : AT (ADDR (.rodata) - 0xC0000000)
    {
        _rodata_start = .;
        *(.rodata)
        _rodata_end = .;
    }

	.data ALIGN (4K) : AT (ADDR (.data) - 0xC0000000)
	{
		_data_start = .;
		*(.data)
		_data_end = .;
	}
	
	.bss ALIGN (4K) : AT (ADDR (.bss) - 0xC0000000)
	{
		_bss_start = .;
		*(COMMON)
		*(.bss)
		*(.bootstrap_stack)
		_bss_end = .;
	}
	/* Add a symbol that indicates the end address of the kernel. */
	_kernel_end = .;
}