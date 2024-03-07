#include <stdint.h>

struct spinlock {
    uint32_t locked;
    char[8] name;
} 

typedef struct spinlock spinlock_t; 