// pci.c — PCI/PCIe driver FIXED
#include "kernel.h"

#define CONFIG_ADDR         0xCF8
#define CONFIG_DATA         0xCFC

static struct pci_device devices[MAX_PCI_DEVICES];
static size_t device_count = 0;

static uint32_t pci_read(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t addr = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) |
                (offset & 0xFC) | 0x80000000);
    outl(CONFIG_ADDR, addr);
    return inl(CONFIG_DATA);
}

static void pci_write(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t val) {
    uint32_t addr = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) |
                (offset & 0xFC) | 0x80000000);
    outl(CONFIG_ADDR, addr);
    outl(CONFIG_DATA, val);
}

void pci_init(void) {
    kprintf("PCI: Scanning buses...\n");
    
    // Scan all buses
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            uint32_t vendor = pci_read(bus, slot, 0, 0);
            if ((vendor & 0xFFFF) == 0xFFFF) continue;
            
            uint8_t header = (pci_read(bus, slot, 0, 0x0E) >> 16) & 0xFF;
            uint8_t funcs = (header & 0x80) ? 8 : 1;
            
            for (uint8_t func = 0; func < funcs; func++) {
                vendor = pci_read(bus, slot, func, 0);
                if ((vendor & 0xFFFF) == 0xFFFF) continue;
                
                if (device_count >= MAX_PCI_DEVICES) break;
                
                struct pci_device* dev = &devices[device_count++];
                dev->bus = bus;
                dev->slot = slot;
                dev->func = func;
                dev->vendor_id = vendor & 0xFFFF;
                dev->device_id = vendor >> 16;
                
                uint32_t class = pci_read(bus, slot, func, 0x08);
                dev->class_code = (class >> 24) & 0xFF;
                dev->subclass = (class >> 16) & 0xFF;
                dev->prog_if = (class >> 8) & 0xFF;
                dev->revision = class & 0xFF;
                
                // Read IRQ
                dev->irq_line = pci_read(bus, slot, func, 0x3C) & 0xFF;
                dev->irq_pin = (pci_read(bus, slot, func, 0x3D) >> 8) & 0xFF;
                
                // Read BARs
                for (int i = 0; i < 6; i++) {
                    uint32_t old = pci_read(bus, slot, func, 0x10 + i * 4);
                    pci_write(bus, slot, func, 0x10 + i * 4, 0xFFFFFFFF);
                    uint32_t sz = pci_read(bus, slot, func, 0x10 + i * 4);
                    pci_write(bus, slot, func, 0x10 + i * 4, old);
                    
                    if (sz == 0 || sz == 0xFFFFFFFF) {
                        dev->bar[i] = 0;
                        dev->bar_phys[i] = 0;
                        dev->bar_size[i] = 0;
                        dev->bar_is_mmio[i] = false;
                        continue;
                    }
                    
                    uint64_t size = (~(sz & 0xFFFFFFF0)) + 1;
                    dev->bar_size[i] = size;
                    dev->bar_is_mmio[i] = !(old & 1);
                    
                    if (old & 0x04) { // 64-bit
                        uint32_t hi = pci_read(bus, slot, func, 0x10 + (i + 1) * 4);
                        dev->bar_phys[i] = ((uint64_t)hi << 32) | (old & 0xFFFFFFF0);
                        dev->bar[i] = old;
                        i++; // Skip high DWORD
                    } else {
                        dev->bar_phys[i] = old & 0xFFFFFFF0;
                        dev->bar[i] = old;
                    }
                }
                
                // Check MSI capability
                dev->msi_capable = false;
                uint8_t cap_ptr = pci_read(bus, slot, func, 0x34) & 0xFF;
                while (cap_ptr) {
                    uint8_t cap_id = pci_read(bus, slot, func, cap_ptr) & 0xFF;
                    if (cap_id == 0x05) { // MSI
                        dev->msi_capable = true;
                        dev->msi_offset = cap_ptr;
                    }
                    cap_ptr = (pci_read(bus, slot, func, cap_ptr) >> 8) & 0xFF;
                    if (cap_ptr == 0) break;
                }
                
                kprintf("PCI %02x:%02x.%x: %04X:%04X class %02X%02X%02X\n",
                    bus, slot, func, dev->vendor_id, dev->device_id,
                    dev->class_code, dev->subclass, dev->prog_if);
            }
        }
    }
    
    kprintf("PCI: %zu devices found\n", device_count);
}

struct pci_device* pci_find_class(uint32_t class_code) {
    for (size_t i = 0; i < device_count; i++) {
        uint32_t cc = ((uint32_t)devices[i].class_code << 16) |
                      ((uint32_t)devices[i].subclass << 8) |
                      devices[i].prog_if;
        if (cc == class_code) return &devices[i];
    }
    return NULL;
}

struct pci_device* pci_find_device(uint16_t vendor, uint16_t device) {
    for (size_t i = 0; i < device_count; i++) {
        if (devices[i].vendor_id == vendor && devices[i].device_id == device) {
            return &devices[i];
        }
    }
    return NULL;
}

struct pci_device* pci_get_device(size_t idx) {
    if (idx < device_count) return &devices[idx];
    return NULL;
}

size_t pci_get_device_count(void) {
    return device_count;
}

uint32_t pci_read_dword(struct pci_device* dev, uint8_t offset) {
    return pci_read(dev->bus, dev->slot, dev->func, offset);
}

uint16_t pci_read_word(struct pci_device* dev, uint8_t offset) {
    uint32_t dw = pci_read(dev->bus, dev->slot, dev->func, offset & ~3);
    return (offset & 2) ? (dw >> 16) : (dw & 0xFFFF);
}

void pci_write_word(struct pci_device* dev, uint8_t offset, uint16_t val) {
    uint32_t dw = pci_read_dword(dev, offset & ~3);
    if (offset & 2) {
        dw = (dw & 0xFFFF) | ((uint32_t)val << 16);
    } else {
        dw = (dw & 0xFFFF0000) | val;
    }
    uint32_t addr = (uint32_t)((dev->bus << 16) | (dev->slot << 11) |
                (dev->func << 8) | (offset & 0xFC) | 0x80000000);
    outl(CONFIG_ADDR, addr);
    outl(CONFIG_DATA, dw);
}

uint64_t pci_read_bar(struct pci_device* dev, uint8_t bar) {
    if (bar >= 6) return 0;
    return dev->bar_phys[bar];
}

void pci_enable_bus_mastering(struct pci_device* dev) {
    uint16_t cmd = pci_read_word(dev, 0x04);
    pci_write_word(dev, 0x04, cmd | PCI_CMD_BUS_MASTER | PCI_CMD_MEM_SPACE);
}

bool pci_enable_msi(struct pci_device* dev, uint8_t vector) {
    if (!dev->msi_capable) return false;
    
    uint16_t msg_ctrl = pci_read_word(dev, dev->msi_offset + 0x02);
    bool is_64bit = msg_ctrl & 0x80;
    
    // Write message address (APIC ID 0, vector)
    uint64_t msg_addr = 0xFEE00000 | (0 << 12);
    pci_write_word(dev, dev->msi_offset + 0x04, msg_addr & 0xFFFF);
    pci_write_word(dev, dev->msi_offset + 0x06, msg_addr >> 16);
    
    if (is_64bit) {
        pci_write_word(dev, dev->msi_offset + 0x08, msg_addr >> 32);
        pci_write_word(dev, dev->msi_offset + 0x0C, vector);
    } else {
        pci_write_word(dev, dev->msi_offset + 0x08, vector);
    }
    
    // Enable MSI
    msg_ctrl |= 0x01;
    pci_write_word(dev, dev->msi_offset + 0x02, msg_ctrl);
    
    // Disable INTx
    uint16_t cmd = pci_read_word(dev, 0x04);
    pci_write_word(dev, 0x04, cmd | 0x0400);
    
    return true;
}
