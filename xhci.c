// xhci.c — XHCI USB driver stub
#include "kernel.h"

bool xhci_init(void) {
    struct pci_device* dev = pci_find_class(PCI_CLASS_XHCI);
    if (!dev) {
        kprintf("XHCI: Controller not found\n");
        return false;
    }
    
    kprintf("XHCI: Found at %02x:%02x.%x\n", dev->bus, dev->slot, dev->func);
    kprintf("XHCI: Driver stub\n");
    
    return true;
}

void xhci_poll(void) {
    // Would poll for USB events
}
