# Declare constants for the multiboot header.
.set ALIGN,    1<<0             # align loaded modules on page boundaries
.set MEMINFO,  1<<1             # provide memory map
.set FLAGS,    ALIGN | MEMINFO  # this is the Multiboot 'flag' field
.set MAGIC,    0x1BADB002       # 'magic number' lets bootloader find the header
.set CHECKSUM, -(MAGIC + FLAGS) # checksum of above, to prove we are multiboot

# Declare multiboot header.
.section .multiboot.data, "aw"
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

# Stack for bootstrapping.
# Allocate the initial stack.
.section .bootstrap_stack, "aw", @nobits
stack_bottom:
.skip 16384 
stack_top:

# Data section.
# Debug info.
.section .data
.global ks
ks:
	.long _kernel_start
.global ke
ke:
	.long _kernel_end
.global ts
ts:
	.long _text_start
.global te
te:
	.long _text_end
.global rs
rs:
	.long _rodata_start
.global re
re:
	.long _rodata_end
.global dtas
dtas:
	.long _data_start
.global dtae
dtae:
	.long _data_end
.global bs
bs:
	.long _bss_start
.global be
be:
	.long _bss_end
.global test
test:
	.long _test

# BSS (block starting symbol) section.
# For page directory and tables.
.section .bss, "aw", @nobits
	.align 4096
# Allocate page directory, 4Mib table frames * 1024 entries = 4GiB. 
# 1024 entries * 4 bytes per = 4096 bytes.
.global boot_page_directory
boot_page_directory:
	.skip 4096
# Allocate page table, 4KiB page frames * 1024 entries = 4MiB. 
# 1024 entries * 4 bytes per = 4096 bytes.
.global boot_page_table
boot_page_table:
	.skip 4096

# The kernel entry point.
.section .multiboot.text, "a"
.global _start
.type _start, @function
_start:
	# Move physical address of boot_page_table into edi.
	movl $(boot_page_table - 0xC0000000), %edi
	movl $0, %esi
	# Map 1023 pages. 1024 is for the VGA text buffer.
	movl $1023, %ecx

0:
	movl %esi, %edx
	orl $0x003, %edx
	cmpl $_kernel_start, %esi
	jb clear_rw
	cmpl $(_text_start - 0xC0000000), %esi
	jb 1f
	cmpl $(_rodata_end - 0xC0000000), %esi
	jae 1f
clear_rw:	
	andl $0xFFFFFFFD, %edx

1:
	# Put the table entry in edx into the page table.
	movl %edx, (%edi)

2:
	# Size of page is 4096 bytes.
	addl $0x1000, %esi
	# Size of entries in boot_page_table is 4 bytes.
	addl $4, %edi
	# Loop to the next entry if we haven't finished.
	loop 0b

3:
	# Map VGA text buffer at 0xB8000 to 0xC03FF000 as "present, writable".
	# Need to be able to write to the VGA text buffer.
	movl $(0x000B8000 | 0x003), boot_page_table - 0xC0000000 + 1023 * 4

	# The page table is used at both page directory entry 0 (virtually from 0x0
	# to 0x3FFFFF) (thus identity mapping the kernel) and page directory entry
	# 768 (virtually from 0xC0000000 to 0xC03FFFFF) (thus mapping it in the
	# higher half). The kernel is identity mapped because enabling paging does
	# not change the next instruction, which continues to be physical. The CPU
	# would instead page fault if there was no identity mapping.

	# Map the page table to both virtual addresses 0x00000000-0x003FFFFF and 0xC0000000-0xC03FFFFF.
	# Set as "present", "writable" & clear low 12 bits.
	movl $boot_page_table - 0xC0000000, %edx
	andl $0xFFFFF000, %edx
	orl $0x003, %edx
	# Put the physical address in edx into the page directory.
	movl %edx, boot_page_directory - 0xC0000000
	# Put the physical address in edx into the page directory.
	movl %edx, boot_page_directory - 0xC0000000 + 768 * 4

	# Set cr3 to the address of the boot_page_directory.
	movl $(boot_page_directory - 0xC0000000), %ecx
	movl %ecx, %cr3

	# Enable paging and the write-protect bit.
	movl %cr0, %ecx
	orl $0x80010000, %ecx
	movl %ecx, %cr0

	# Jump to higher half with an absolute jump. 
	lea 4f, %ecx
	jmp *%ecx

.section .text

4:

	# Unmap unnecessary identity paging. 
	movl $0, boot_page_directory + 0

	# Reload cr3 to force a TLB flush so the changes to take effect.
	movl %cr3, %ecx
	movl %ecx, %cr3

	# Set up the stack.
	mov $stack_top, %esp
	pushl %eax
	pushl %ebx

	# Enter the high-level kernel.
	call kmain

	# Infinite loop if the system has nothing more to do.
	cli
1:	hlt
	jmp 1b