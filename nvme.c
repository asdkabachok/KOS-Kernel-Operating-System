// nvme.c — NVMe driver stub
#include "kernel.h"

static bool nvme_initialized = false;
static uint64_t boot_drive_size = 0;

bool nvme_init(void) {
    struct pci_device* dev = pci_find_class(PCI_CLASS_NVME);
    if (!dev) {
        kprintf("NVMe: Controller not found\n");
        return false;
    }
    
    kprintf("NVMe: Found at %02x:%02x.%x\n", dev->bus, dev->slot, dev->func);
    kprintf("NVMe: Driver stub - full driver not implemented\n");
    
    // Return true to indicate controller found
    // but don't claim boot drive is ready
    nvme_initialized = true;
    boot_drive_size = 500ULL * 1024 * 1024 * 1024; // Assume 500GB
    
    return true;
}

uint64_t nvme_get_boot_size(void) {
    return boot_drive_size;
}

bool nvme_read_boot(uint64_t lba, uint32_t count, void* buf) {
    if (!nvme_initialized) return false;
    
    // Stub - would need full NVMe driver
    kprintf("NVMe: Read %u sectors at LBA %lu (not implemented)\n", count, lba);
    memset(buf, 0, count * 512);
    return false;
}

bool nvme_write_boot(uint64_t lba, uint32_t count, const void* buf) {
    if (!nvme_initialized) return false;
    
    // Stub
    (void)lba;
    (void)count;
    (void)buf;
    return false;
}
