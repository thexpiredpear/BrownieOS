#ifndef _KERNEL_TTY_H
#define _KERNEL_TTY_H

#include <stddef.h>

// Initializes the kernel TTY to write to the VGA text-mode buffer mapped in
// the kernel window. Clears the screen and sets the default color attributes.
void terminal_initialize(void);
// Writes a single character to the current cursor position, handling newline
// and scrolling. Does not perform locking; intended for early kernel output.
void terminal_putchar(char c);
// Writes `size` bytes from `data` to the terminal, invoking `terminal_putchar`
// for each byte. `data` is a kernel virtual pointer.
void terminal_write(const char* data, size_t size);
// Convenience wrapper to write a NUL-terminated string.
void terminal_writestring(const char* data);

#endif
