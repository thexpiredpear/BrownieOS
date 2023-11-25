#include <string.h>

int strcmp(const char* aptr, const char* bptr) {
    const unsigned char* a = (const unsigned char*) aptr;
    const unsigned char* b = (const unsigned char*) bptr;
    for (size_t i = 0; ; i++) {
        if (a[i] < b[i])
            return -1;
        else if (b[i] < a[i])
            return 1;
        else if (a[i] == '\0')
            return 0;
    }
}