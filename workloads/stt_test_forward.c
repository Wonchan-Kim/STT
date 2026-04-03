#include <stdio.h>
#include <stdint.h>

volatile uint64_t g = 0;

int main(void) {
    uint64_t x = 0;

    for (int i = 0; i < 1000; i++) {
        g = (uint64_t)i + 1;   // store
        x = g;                 // load, should be a forwarding candidate
        asm volatile("" : : "r"(x) : "memory");
    }

    return (int)x;
}
