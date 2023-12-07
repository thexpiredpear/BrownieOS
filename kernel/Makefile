KERNEL_VERSION=0.0.1
KERNEL_ARCH=x86

DEFAULT_HOST!=../default-host.sh
HOST?=DEFAULT_HOST
HOSTARCH!=../target-triplet-to-arch.sh $(HOST)

CFLAGS?=-O0 -g
CPPFLAGS?=
LDFLAGS?=
LIBS?=

DESTDIR?=
PREFIX?=/usr
BOOTDIR?=$(PREFIX)/boot
INCLUDEDIR?=$(PREFIX)/include

CFLAGS:=$(CFLAGS) -ffreestanding -Wall -Wextra
CPPFLAGS:=$(CPPFLAGS) -D__is_kernel -Iinclude
LDFLAGS:=$(LDFLAGS)
LIBS:=$(LIBS) -nostdlib -lk -lgcc

ARCHDIR=

include make.config

CFLAGS:=$(CFLAGS) $(KERNEL_ARCH_CFLAGS)
CPPFLAGS:=$(CPPFLAGS) $(KERNEL_ARCH_CPPFLAGS)
LDFLAGS:=$(LDFLAGS) $(KERNEL_ARCH_LDFLAGS)
LIBS:=$(LIBS) $(KERNEL_ARCH_LIBS)

KERNEL_OBJS=\
kernel/boot/crt0.o \
kernel/core/kernel.o \
kernel/core/common.o \
kernel/core/gdtflush.o \
kernel/core/gdt.o \
kernel/core/acpi.o \
kernel/core/madt.o \
kernel/core/interrupts/idtflush.o \
kernel/core/interrupts/interrupt.o \
kernel/core/interrupts/isr.o \
kernel/core/interrupts/idt.o \
kernel/core/interrupts/apic.o \
kernel/core/interrupts/ioapic.o \
kernel/drivers/tty.o \
kernel/drivers/pit.o \
kernel/mm/kmm.o \
kernel/mm/paging.o \
kernel/mm/vmm.o \

OBJS=\
kernel/boot/crti.o \
crtbegin.o \
$(KERNEL_OBJS) \
crtend.o \
kernel/boot/crtn.o \

LINK_LIST=\
$(LDFLAGS) \
kernel/boot/crti.o \
crtbegin.o \
$(KERNEL_OBJS) \
$(LIBS) \
crtend.o \
kernel/boot/crtn.o \
# \
CFLAGS = -O2 -g -ffreestanding -Wall -Wextra \
CPPFLAGS = -D__is_kernel -Iinclude \
LDFLAGS =  \
LIBS = -nostdlib -lk -lgcc \
\
ARCHDIR = \
INCLUDEDIR = /usr/local/include\
BOOTDIR = /usr/local/boot\
\
OBJS = \
	crti.o \
	crtbegin.o \
	crt0.o \
	tty.o \
	common.o \
	kernel/kernel.o \
	crtend.o \
	crtn.o \

.PHONY: all clean install install-headers install-kernel
.SUFFIXES: .o .c .S

all: brownieos.kernel

brownieos.kernel: $(OBJS) linker.ld
	$(CC) -T linker.ld -o $@ $(CFLAGS) $(LINK_LIST)
	grub-file --is-x86-multiboot brownieos.kernel

crtbegin.o crtend.o:
	OBJ=`$(CC) $(CFLAGS) $(LDFLAGS) -print-file-name=$(@F)` && cp "$$OBJ" $@

.c.o:
	$(CC) -MD -DKERNEL_VERSION=\"$(KERNEL_VERSION)\" -DKERNEL_ARCH=\"$(KERNEL_ARCH)\" -c $< -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)

.S.o:
	$(CC) -MD -c $< -o $@ $(CFLAGS) $(CPPFLAGS)

install: install-headers install-kernel

install-headers:
	mkdir -p $(DESTDIR)$(INCLUDEDIR)
	cp -R --preserve=timestamps include/. $(DESTDIR)$(INCLUDEDIR)/.

install-kernel: brownieos.kernel
	mkdir -p $(DESTDIR)$(BOOTDIR)
	cp brownieos.kernel $(DESTDIR)$(BOOTDIR)

clean:
	rm -f brownieos.kernel
	rm -f $(OBJS) *.o */*.o */*/*.o
	rm -f $(OBJS:.o=.d) *.d */*.d */*/*.d

-include $(OBJS:.o=.d)
