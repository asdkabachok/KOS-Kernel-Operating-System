// ioapic.c — IO APIC driver stub
#include "kernel.h"

static volatile uint32_t ioapic_bases[MAX_IOAPICS];
static uint32_t gsi_bases[MAX_IOAPICS];
static uint8_t num_ioapics = 0;

void ioapic_init(void) {
    num_ioapics = acpi_get_ioapic_count();
    
    if (num_ioapics == 0) {
        // Default IOAPIC
        num_ioapics = 1;
        ioapic_bases[0] = 0xFEC00000;
        gsi_bases[0] = 0;
    }
    
    for (uint8_t i = 0; i < num_ioapics; i++) {
        uint32_t phys = acpi_get_ioapic_addr(i);
        if (phys == 0) phys = 0xFEC00000;
        
        gsi_bases[i] = acpi_get_ioapic_gsi_base(i);
        
        kprintf("IOAPIC[%u]: addr=0x%X, gsi=%u\n", i, phys, gsi_bases[i]);
    }
    
    kprintf("IOAPIC: %u IOAPICs initialized\n", num_ioapics);
}

void ioapic_set_irq(uint8_t irq, uint8_t vector, uint32_t dest) {
    (void)irq;
    (void)vector;
    (void)dest;
    // Would configure IOAPIC redirection entry
}

void ioapic_mask_irq(uint8_t irq) {
    (void)irq;
}

void ioapic_unmask_irq(uint8_t irq) {
    (void)irq;
}
