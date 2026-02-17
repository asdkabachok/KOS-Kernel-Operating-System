// main.c — KOS 1.0 Final for Acer Nitro AN515-43
#include "kernel.h"

// Forward declarations for external functions
extern void gdt64_init(void);
extern void init_tss(void);
extern void idt_init(void);
extern void lapic_init(void);
extern void lapic_timer_calibrate(void);
extern void ioapic_init(void);
extern void smp_init(void);
extern void scheduler_init(void);
extern void pci_init(void);
extern void lapic_eoi(void);

static uint8_t* find_rsdp(uint64_t mb_info_phys) {
    uint8_t* ptr = (uint8_t*)PHYS_TO_VIRT(mb_info_phys);
    // Skip to first tag (after Multiboot2 header)
    ptr = (uint8_t*)((uint64_t)ptr + 8);
    while (1) {
        struct multiboot_tag* tag = (struct multiboot_tag*)ptr;
        if (tag->type == 0) break;
        if (tag->type == 14 || tag->type == 15) {
            return ((struct multiboot_tag_acpi*)tag)->rsdp;
        }
        ptr += (tag->size + 7) & ~7;
    }
    return NULL;
}

void kernel_main(uint64_t mb_info_phys) {
    console_early_init();
    
    kprintf("\n");
    kprintf("========================================================\n");
    kprintf("  KOS 1.0 Final                                        \n");
    kprintf("  Acer Nitro 5 AN515-43 | Ryzen 5 3550H | 16GB | 500GB\n");
    kprintf("========================================================\n\n");
    
    // Core initialization
    pmm_init(mb_info_phys);
    vmm_init();
    kmalloc_init();
    gdt64_init();
    init_tss();
    idt_init();
    kprintf("[OK] Core initialized\n");
    
    // ACPI
    uint8_t* rsdp = find_rsdp(mb_info_phys);
    if (rsdp && acpi_init(rsdp)) {
        kprintf("[OK] ACPI %u.%u, %u CPUs\n",
            acpi_get_version_major(), acpi_get_version_minor(),
            acpi_get_cpu_count());
    } else {
        kprintf("[WARN] ACPI not found, using defaults\n");
    }
    
    // APIC
    lapic_init();
    lapic_timer_calibrate();
    ioapic_init();
    sti();
    
    // SMP
    cpu_init_early();
    smp_init();
    scheduler_init();
    
    // PCI scan
    pci_init();
    
    // Storage: NVMe (твой 500GB SSD)
    kprintf("\n[STORAGE] NVMe boot drive...\n");
    if (nvme_init()) {
        uint64_t size_gb = nvme_get_boot_size() / (1024 * 1024 * 1024);
        kprintf("[OK] NVMe: %lu GB\n", size_gb);
        
        // Try to mount root filesystem (assuming partition starts at LBA 2048)
        kprintf("[FS] Mounting ext2 root...\n");
        if (ext2_mount(nvme_read_boot, nvme_write_boot, 2048)) {
            kprintf("[OK] Root filesystem mounted\n");
            
            // Test read
            struct ext2_file init_file;
            if (ext2_open("/sbin/init", &init_file)) {
                kprintf("[OK] Found /sbin/init, size %u bytes\n", init_file.inode.i_size);
            }
        }
    } else {
        kprintf("[WARN] NVMe not initialized, trying AHCI...\n");
        if (ahci_init()) {
            kprintf("[OK] AHCI: %u ports\n", ahci_get_port_count());
        }
    }
    
    // Network: Realtek r8168 (preferred) or Intel e1000e
    kprintf("\n[NETWORK] Ethernet controllers...\n");
    if (r8168_init()) {
        kprintf("[OK] r8168: Gigabit link ready\n");
    } else if (e1000e_init()) {
        kprintf("[OK] e1000e: Intel Gigabit\n");
    } else {
        kprintf("[WARN] No Ethernet controller found\n");
    }
    
    // Audio: HDA with Realtek ALC
    kprintf("\n[AUDIO] HD Audio...\n");
    if (hda_init()) {
        kprintf("[OK] HDA audio ready\n");
    } else {
        kprintf("[WARN] HDA not initialized\n");
    }
    
    // USB: XHCI
    kprintf("\n[USB] XHCI controller...\n");
    if (xhci_init()) {
        kprintf("[OK] XHCI initialized\n");
    }
    
    // GPU: Vega 8
    kprintf("\n[GPU] AMD Radeon Vega 8...\n");
    if (amdgpu_init()) {
        amdgpu_clear(0x000011); // Dark blue background
        kprintf("[OK] Framebuffer %ux%u\n",
            amdgpu_get_fb_width(), amdgpu_get_fb_height());
    } else {
        kprintf("[INFO] Using VGA text mode\n");
    }
    
    // Laptop specific
    kprintf("\n[LAPTOP] Hardware monitoring...\n");
    laptop_ec_init();
    laptop_thermal_init();
    kprintf("     Temp: %uC, Fan: %u RPM\n",
        laptop_get_cpu_temp(), laptop_get_fan_rpm());
    
    // Final status
    kprintf("\n========================================================\n");
    kprintf("[OK] KOS ready on %u CPUs\n", smp_get_cpu_count());
    kprintf("     Memory: %lu MB free\n", pmm_get_free() / (1024 * 1024));
    kprintf("========================================================\n\n");
    
    // Idle loop with polling
    while (1) {
        // Poll network
        network_poll();
        
        // Poll laptop thermal
        laptop_thermal_poll();
        
        // Halt to save power
        hlt();
    }
}
