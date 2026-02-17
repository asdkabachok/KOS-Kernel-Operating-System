// ahci.c — AHCI SATA driver (stub)
#include "kernel.h"

static uint8_t port_count = 0;

bool ahci_init(void) {
    struct pci_device* dev = pci_find_class(PCI_CLASS_AHCI);
    if (!dev) return false;
    
    kprintf("AHCI: Controller found at %02x:%02x.%x\n",
        dev->bus, dev->slot, dev->func);
    
    port_count = 4; // Assume 4 ports
    kprintf("AHCI: %u ports\n", port_count);
    
    return true;
}

bool ahci_read_sectors(uint8_t port, uint64_t lba, uint32_t count, void* buf) {
    (void)port;
    (void)lba;
    (void)count;
    (void)buf;
    return false;
}

bool ahci_write_sectors(uint8_t port, uint64_t lba, uint32_t count, const void* buf) {
    (void)port;
    (void)lba;
    (void)count;
    (void)buf;
    return false;
}

uint8_t ahci_get_port_count(void) {
    return port_count;
}
