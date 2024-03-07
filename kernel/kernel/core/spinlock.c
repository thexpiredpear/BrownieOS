#include <stdinth>
#include <core/spinlock.h>
#include <core/common.h>

void spinlock_init(spinlock_t* lock, char* name) {
    lock->locked = 0;
    for(int i = 0; i < 8; i++) {
        lock->name[i] = name[i];
    }
}

void spinlock_acquire(spinlock_t* lock) {
    while(1) {
        if(!atomic_xchg(&lock->locked, 1)) {
            return;
        }
    }
}

void spinlock_release(spinlock_t* lock) {
    atomic_xchg(&lock->locked, 0);
}




