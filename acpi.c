// acpi.c — ACPI parser stub
#include "kernel.h"

static bool acpi_init_done = false;
static uint8_t acpi_major = 0, acpi_minor = 0;
static uint32_t num_cpus = 1;
static uint8_t cpu_apic_ids[MAX_CPUS];
static uint32_t num_ioapics = 1;
static uint32_t ioapic_addrs[MAX_IOAPICS] = {0xFEC00000};
static uint32_t ioapic_gsi_bases[MAX_IOAPICS] = {0};

bool acpi_init(uint8_t* rsdp) {
    (void)rsdp;
    
    kprintf("ACPI: Initializing (stub)\n");
    
    // Assume single core for now
    cpu_apic_ids[0] = 0;
    num_cpus = 1;
    
    acpi_init_done = true;
    acpi_major = 5;
    acpi_minor = 0;
    
    kprintf("ACPI: Version %u.%u, %u CPUs\n", acpi_major, acpi_minor, num_cpus);
    
    return true;
}

void acpi_shutdown(void) {
    kprintf("ACPI: Shutdown (stub)\n");
    while (1) hlt();
}

void acpi_reboot(void) {
    kprintf("ACPI: Reboot (stub)\n");
    // Would try various reboot methods
    while (1) hlt();
}

bool acpi_initialized(void) { return acpi_init_done; }
uint8_t acpi_get_version_major(void) { return acpi_major; }
uint8_t acpi_get_version_minor(void) { return acpi_minor; }
uint32_t acpi_get_cpu_count(void) { return num_cpus; }
uint8_t acpi_get_cpu_apic_id(uint32_t idx) { 
    return (idx < num_cpus) ? cpu_apic_ids[idx] : 0; 
}
uint32_t acpi_get_ioapic_count(void) { return num_ioapics; }
uint32_t acpi_get_ioapic_addr(uint32_t idx) { 
    return (idx < num_ioapics) ? ioapic_addrs[idx] : 0; 
}
uint32_t acpi_get_ioapic_gsi_base(uint32_t idx) { 
    return (idx < num_ioapics) ? ioapic_gsi_bases[idx] : 0; 
}
