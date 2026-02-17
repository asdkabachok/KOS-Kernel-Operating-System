// apic.c — Local APIC driver stub
#include "kernel.h"

static volatile uint32_t* lapic_base = NULL;
static uint64_t timer_freq = 0;
static void (*timer_handler)(void) = NULL;

void lapic_init(void) {
    // Disable legacy PIC
    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);
    
    // Check CPUID for APIC
    uint32_t eax, ebx, ecx, edx;
    cpuid(1, &eax, &ebx, &ecx, &edx);
    if (!(edx & (1 << 9))) {
        kernel_panic("APIC not supported");
    }
    
    // Enable APIC via MSR
    uint64_t apic_base = rdmsr(0x1B);
    if (!(apic_base & (1 << 11))) {
        wrmsr(0x1B, apic_base | (1 << 11));
    }
    
    // Map LAPIC (typically at 0xFEE00000)
    uint64_t phys_base = apic_base & 0xFFFFF000;
    lapic_base = (volatile uint32_t*)PHYS_TO_VIRT(phys_base);
    
    // Software enable
    lapic_base[0x0F0 / 4] = 0x1FF;
    
    kprintf("LAPIC: ID=%u, Version=%u\n", 
        lapic_base[0x020 / 4] >> 24, 
        lapic_base[0x030 / 4] & 0xFF);
}

void lapic_init_ap(void) {
    // AP initialization
    lapic_base[0x0F0 / 4] = 0x1FF;
    lapic_base[0x080 / 4] = 0;
    lapic_eoi();
}

uint32_t lapic_get_id(void) {
    if (!lapic_base) return 0;
    return lapic_base[0x020 / 4] >> 24;
}

void lapic_eoi(void) {
    if (lapic_base) {
        lapic_base[0x0B0 / 4] = 0;
    }
}

void lapic_send_ipi(uint8_t apic_id, uint8_t vector) {
    if (!lapic_base) return;
    
    lapic_base[0x310 / 4] = ((uint32_t)apic_id) << 24;
    lapic_base[0x300 / 4] = vector | 0x4000;
}

void lapic_timer_calibrate(void) {
    kprintf("LAPIC: Timer calibration (stub)\n");
    timer_freq = 1000000; // Assume 1MHz for now
}

uint64_t lapic_get_timer_freq(void) { return timer_freq; }
uint64_t lapic_get_timer_ticks(void) { 
    if (!lapic_base) return 0;
    return lapic_base[0x390 / 4]; 
}

void lapic_timer_set_handler(void (*handler)(void)) {
    timer_handler = handler;
}

void lapic_timer_start_periodic(uint64_t ms) {
    (void)ms;
    // Would configure periodic timer
}
