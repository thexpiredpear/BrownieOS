# BrownieOS

## Overview

An (attempted) 32-bit, x86, *nix-like kernel and operating system for fun and learning. 

Generally, meant as a hobby OS with a clean, easy to understand structure to help myself and others learn about operating system fundamentals.

Early parts of kernel (paging initialization, terminal, interrupts w/PIC, PIT) heavily sourced from OSDev Wiki and James Molloy's kernel development tutorial. 

## Building

The current build system is messy and runs through pipes of shell scripts, which I'm looking to simplify soon. For now, run ./build.sh to build or ./qemu.sh to build and launch in qemu-system-i386

## Roadmap

### 1: Pre-Userspace

- [x] Initialize System
    - [x] Boot
    - [x] GDT
    - [x] PIC & IDT
    - [x] Paging

- [x] Basic Functionality
    - [x] Terminal
    - [x] PIT
    - [x] Parse ACPI Tables

- [x] Kernel Memory
    - [x] PMM
    - [x] VMM
    - [x] kmalloc()

- [ ] Multitasking
    - [ ] APIC
        - [ ] Parse MADT
        - [ ] LAPIC
        - [ ] IOAPIC
    - [ ] HPET
    - [ ] Context Switch
    - [ ] Scheduler

- [ ] Userspace

## Credits

BrownieOS borrows code and takes inspiration from

### OSDev Wiki

<https://wiki.osdev.org>

### James Molloy's Kernel Development Tutorials

<http://www.jamesmolloy.co.uk/tutorial_html/>
