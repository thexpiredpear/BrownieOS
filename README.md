# BrownieOS

## Overview

An (attempted) 32-bit, x86, *nix-like kernel and operating system for fun and learning. 

Generally, meant as a hobby OS with a clean, easy to understand structure to help myself and others learn about operating system fundamentals.

Early parts of kernel (paging initialization, terminal, interrupts w/PIC, PIT) heavily sourced from OSDev Wiki and James Molloy's kernel development tutorial. 

## Dependencies 

### THIS CODE HAS ONLY BEEN COMPILED ON UBUNTU AND TESTED ON QEMU!

To build and run BrownieOS, you will need a cross compiler toolchain that targets `i686-elf`. To use the provided build script, you will also need to install  `xorriso` and `grub-pc-bin`. If not using `i686-elf-gcc/as/ar` as your invocations, you will need to edit `config.sh`. You will also need an emulator to run if not using real hardware - BrownieOS has only been tested on `qemu-system-i386`.

You can use `install-ubuntu-dependencies.sh` to easy install a `i686-elf` toolchain and other dependencies, but this has only been tested on Ubuntu 22.04.3 LTS.

## Building

The current build system is messy and runs through pipes of shell scripts, which I'm looking to simplify soon. For now, run `./build.sh` to build or `./qemu.sh` to build and launch in `qemu-system-i386`.

The shell script build.sh will create a sysroot dir which the final iso image is then compiled from. You can then run that in any `ix86` emulator of your choice (should be compatible across i386 - i686) - preferably QEMU. The physical memory manager assumes `4GiB` of physical memory available. If possible, emulate with between `2-8` processors.

## Roadmap

### 1: Pre-Userspace
#### I've decided against enabling SMP for now, but will implement concurrency primitives - the (IO)APIC complexity is unnecessary 
- [x] Initialize System
    - [x] Boot
    - [x] GDT
    - [x] PIC & IDT
    - [x] Paging

- [x] Basic Functionality
    - [x] Terminal
    - [x] PIT
    - [x] Parse ACPI Tables

- [x] Kernel Memory Management
    - [x] PMM
    - [x] VMM
    - [x] kmalloc()

- [x] Multitasking (Finished as of 11/18)
    - [x] APIC
        - [x] Parse MADT
        - [x] LAPIC
        - [x] IOAPIC
    - [x] HPET
    - [x] Abstract Userspace Process
    - [x] Context Switch
    - [x] Scheduler

- [ ] LibC expansion

## Credits

BrownieOS borrows code and/or inspiration from the following projects

### GRUB (GRand Unified Bootloader)

<https://www.gnu.org/software/grub/grub.html>

### OSDev Wiki

<https://wiki.osdev.org>

### James Molloy's Kernel Development Tutorials

<http://www.jamesmolloy.co.uk/tutorial_html/>

### BRUTAL Operating System

<https://github.com/brutal-org/brutal/>

### MIT xv6 for x86

<https://github.com/mit-pdos/xv6-public>
