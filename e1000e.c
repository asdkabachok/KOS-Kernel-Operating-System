// e1000e.c — Intel Gigabit Ethernet stub
#include "kernel.h"

bool e1000e_init(void) {
    // Try to find Intel NIC
    struct pci_device* dev = pci_find_device(0x8086, 0x10D3); // 82574L
    if (!dev) dev = pci_find_device(0x8086, 0x10F6); // 82574LA
    if (!dev) dev = pci_find_device(0x8086, 0x1533); // I210
    if (!dev) dev = pci_find_device(0x8086, 0x1539); // I211
    
    if (!dev) {
        kprintf("e1000e: Intel Ethernet not found\n");
        return false;
    }
    
    kprintf("e1000e: Found at %02x:%02x.%x\n", dev->bus, dev->slot, dev->func);
    kprintf("e1000e: Driver stub\n");
    
    return true;
}
