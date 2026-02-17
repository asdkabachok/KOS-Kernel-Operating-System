// pit.c — Programmable Interval Timer FIXED
#include "kernel.h"

#define PIT_FREQ        1193182
#define PIT_CMD         0x43
#define PIT_CH0         0x40

void pit_wait(uint32_t ms) {
    // Disable interrupts during wait
    cli();
    
    uint16_t count = (PIT_FREQ * ms) / 1000;
    if (count == 0) count = 1;
    
    // Channel 0, lobyte/hibyte, mode 0 (interrupt on terminal count)
    outb(PIT_CMD, 0x30);
    
    outb(PIT_CH0, count & 0xFF);
    outb(PIT_CH0, (count >> 8) & 0xFF);
    
    // Wait for counter to reach zero
    while (count > 0) {
        // Read current count (latch)
        outb(PIT_CMD, 0x00);
        uint16_t current = inb(PIT_CH0) | (inb(PIT_CH0) << 8);
        if (current == 0) current = 0x10000;
        if (current > count) break;
    }
    
    sti();
}

// Simple busy wait for small delays
void pit_delay(uint32_t us) {
    // Approximate - not accurate but sufficient for small delays
    for (uint32_t i = 0; i < us * 3; i++) {
        asm volatile ("nop");
    }
}
