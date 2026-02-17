// kernel.h — KOS 1.0 Core Architecture FIXED
#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>

// Version
#define KOS_VERSION             "KOS 1.0.0-Laptop"
#define KOS_VERSION_MAJOR       1
#define KOS_VERSION_MINOR       0
#define KOS_VERSION_PATCH       0

// Hardware limits (твой ноут: 16GB, 8 threads)
#define MAX_CPUS                8
#define MAX_IOAPICS             4
#define MAX_PCI_DEVICES         256
#define MAX_IRQS                256
#define MAX_MEMORY_GB           16
#define MAX_PAGES               ((uint64_t)MAX_MEMORY_GB * 1024 * 1024 * 1024 / PAGE_SIZE)

// Memory layout
#define KERNEL_HIGHER_HALF      0xFFFF800000000000ULL
#define KERNEL_TEXT_START       0xFFFF800000000000ULL
#define USER_SPACE_START        0x0000000000400000ULL
#define USER_STACK_TOP          0x00007FFFFFFFF000ULL
#define PHYS_TO_VIRT(p)         ((void*)((uint64_t)(p) + KERNEL_HIGHER_HALF))
#define VIRT_TO_PHYS(v)         ((uint64_t)(v) - KERNEL_HIGHER_HALF)
#define PAGE_SIZE               4096
#define HUGE_PAGE_SIZE          (2 * 1024 * 1024)
#define PAGE_MASK               (~(PAGE_SIZE - 1))
#define KERNEL_STACK_SIZE       (PAGE_SIZE * 16)

// Page table flags
#define PT_PRESENT              0x001
#define PT_WRITABLE             0x002
#define PT_USER                 0x004
#define PT_WRITETHROUGH         0x008
#define PT_NOCACHE              0x010
#define PT_ACCESSED             0x020
#define PT_DIRTY                0x040
#define PT_HUGE                 0x080
#define PT_GLOBAL               0x100
#define PT_NX                   0x8000000000000000ULL

// Interrupt vectors
#define IRQ_VECTOR_OFFSET       0x20
#define IRQ_TIMER               0x20
#define IRQ_KEYBOARD            0x21
#define IRQ_COM1                0x24
#define IRQ_ETHERNET            0x25
#define IRQ_SPURIOUS            0xFF
#define SYSCALL_VECTOR          0x80

// Multiboot2
#define MULTIBOOT_TAG_TYPE_END          0
#define MULTIBOOT_TAG_TYPE_MMAP         6
#define MULTIBOOT_TAG_TYPE_ACPI_OLD     14
#define MULTIBOOT_TAG_TYPE_ACPI_NEW     15
#define MULTIBOOT_MEMORY_AVAILABLE      1

// PCI classes
#define PCI_CLASS_AHCI          0x010601
#define PCI_CLASS_NVME          0x010802
#define PCI_CLASS_XHCI          0x0C0330
#define PCI_CLASS_EHCI          0x0C0320
#define PCI_CLASS_HDA           0x040300
#define PCI_CLASS_ETHERNET      0x020000
#define PCI_CLASS_WIFI          0x028000
#define PCI_CLASS_VGA           0x030000

// PCI commands
#define PCI_CMD_BUS_MASTER      0x04
#define PCI_CMD_MEM_SPACE       0x02
#define PCI_CMD_IO_SPACE        0x01

// LAPIC
#define LAPIC_ID                0x020
#define LAPIC_VERSION           0x030
#define LAPIC_TPR               0x080
#define LAPIC_EOI               0x0B0
#define LAPIC_SVR               0x0F0
#define LAPIC_ICR_LOW           0x300
#define LAPIC_ICR_HIGH          0x310
#define LAPIC_LVT_TIMER         0x320
#define LAPIC_LVT_PERF          0x340
#define LAPIC_LVT_LINT0         0x350
#define LAPIC_LVT_LINT1         0x360
#define LAPIC_LVT_ERROR         0x370
#define LAPIC_TIMER_INIT_COUNT  0x380
#define LAPIC_TIMER_CURRENT     0x390
#define LAPIC_TIMER_DIV         0x3E0
#define LAPIC_SVR_ENABLE        0x100
#define LAPIC_SVR_FOCUS_OFF     0x200
#define LAPIC_SVR_APIC_ENABLE   0x001

// x2APIC (Ryzen support)
#define X2APIC_MSR_BASE         0x800
#define X2APIC_MSR_ID           (X2APIC_MSR_BASE + 0x002)
#define X2APIC_MSR_EOI          (X2APIC_MSR_BASE + 0x00B)
#define X2APIC_MSR_SELF_IPI     (X2APIC_MSR_BASE + 0x03F)
#define X2APIC_MSR_ICR          (X2APIC_MSR_BASE + 0x030)

// IOAPIC
#define IOAPIC_REG_SELECT       0x00
#define IOAPIC_REG_WINDOW       0x10
#define IOAPIC_REG_VERSION      0x01

// ACPI Tables
#define ACPI_RSDP_SIGNATURE    0x2052545020445352ULL // "RSDP "
#define ACPI_XSDP_SIGNATURE    0x2054535820445352ULL // "RSDT "
#define ACPI_MADT_SIGNATURE    0x205441444D544F4FULL // "APIC"
#define ACPI_SSDT_SIGNATURE    0x2054545344505353ULL // "SSDT"
#define ACPI_FADT_SIGNATURE    0x2054414643415446ULL // "FACP"

// ACPI MADT types
#define ACPI_MADT_TYPE_LOCAL_APIC           0
#define ACPI_MADT_TYPE_IO_APIC              1
#define ACPI_MADT_TYPE_INTERRUPT_SOURCE     2
#define ACPI_MADT_TYPE_NMI_SOURCE           3
#define ACPI_MADT_TYPE_LOCAL_APIC_NMI       4
#define ACPI_MADT_TYPE_LOCAL_APIC_OVERRIDE  5
#define ACPI_MADT_TYPE_IO_SAPIC             6
#define ACPI_MADT_TYPE_LOCAL_SAPIC          7
#define ACPI_MADT_TYPE_PLATFORM_INT         8
#define ACPI_MADT_TYPE_PROCESSOR_LOCAL_APIC 9
#define ACPI_MADT_TYPE_PROCESSOR_X2APIC     10

// Structures
struct multiboot_tag {
    uint32_t type;
    uint32_t size;
};

struct multiboot_mmap_entry {
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
    uint32_t reserved;
};

struct multiboot_tag_mmap {
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
    struct multiboot_mmap_entry entries[];
};

struct multiboot_tag_acpi {
    uint32_t type;
    uint32_t size;
    uint8_t rsdp[];
};

// Kernel symbols
extern char _kernel_start[];
extern char _kernel_end[];
extern char _code[];
extern char _data[];
extern char _bss[];

// CPU state
struct cpu {
    uint8_t apic_id;
    uint8_t acpi_id;
    uint64_t lapic_base;
    uint64_t tss_rsp0;
    uint64_t current_thread;
    bool online;
    bool bsp;
};

// PCI device
struct pci_device {
    uint8_t bus, slot, func;
    uint16_t vendor_id, device_id;
    uint8_t class_code, subclass, prog_if;
    uint8_t revision;
    uint32_t bar[6];
    uint64_t bar_phys[6];
    uint64_t bar_size[6];
    bool bar_is_mmio[6];
    uint8_t irq_pin, irq_line;
    bool msi_capable;
    uint8_t msi_offset;
};

// Thread/Process
struct thread {
    uint32_t tid;
    uint32_t state;
    uint32_t prio;
    uint64_t vruntime;
    uint64_t rsp;
    uint64_t kernel_stack;
    struct process* parent;
    struct thread* next;
    struct thread* prev;
};

struct process {
    uint32_t pid;
    char name[32];
    uint64_t cr3;
    struct thread main_thread;
};

// Spinlock
typedef struct {
    volatile uint32_t lock;
} spinlock_t;

// Inline utilities
static inline void cli(void) { asm volatile ("cli"); }
static inline void sti(void) { asm volatile ("sti"); }
static inline void hlt(void) { asm volatile ("hlt"); }
static inline void pause(void) { asm volatile ("pause"); }

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    asm volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outw(uint16_t port, uint16_t val) {
    asm volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    asm volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outl(uint16_t port, uint32_t val) {
    asm volatile ("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint64_t rdmsr(uint32_t msr) {
    uint32_t low, high;
    asm volatile ("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return ((uint64_t)high << 32) | low;
}

static inline void wrmsr(uint32_t msr, uint64_t val) {
    uint32_t low = val & 0xFFFFFFFF;
    uint32_t high = val >> 32;
    asm volatile ("wrmsr" : : "a"(low), "d"(high), "c"(msr));
}

static inline uint64_t read_cr0(void) {
    uint64_t val;
    asm volatile ("mov %%cr0, %0" : "=r"(val));
    return val;
}

static inline void write_cr0(uint64_t val) {
    asm volatile ("mov %0, %%cr0" : : "r"(val));
}

static inline uint64_t read_cr2(void) {
    uint64_t val;
    asm volatile ("mov %%cr2, %0" : "=r"(val));
    return val;
}

static inline uint64_t read_cr3(void) {
    uint64_t val;
    asm volatile ("mov %%cr3, %0" : "=r"(val));
    return val;
}

static inline void write_cr3(uint64_t val) {
    asm volatile ("mov %0, %%cr3" : : "r"(val));
}

static inline uint64_t read_cr4(void) {
    uint64_t val;
    asm volatile ("mov %%cr4, %0" : "=r"(val));
    return val;
}

static inline void write_cr4(uint64_t val) {
    asm volatile ("mov %0, %%cr4" : : "r"(val));
}

static inline void cpuid(uint32_t code, uint32_t* a, uint32_t* b, uint32_t* c, uint32_t* d) {
    asm volatile ("cpuid" : "=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d) : "a"(code));
}

static inline void invlpg(uint64_t addr) {
    asm volatile ("invlpg (%0)" : : "r"(addr) : "memory");
}

static inline void mfence(void) {
    asm volatile ("mfence" ::: "memory");
}

static inline void lfence(void) {
    asm volatile ("lfence" ::: "memory");
}

// Byte order swaps
static inline uint16_t bswap16(uint16_t x) {
    return (x >> 8) | (x << 8);
}

static inline uint32_t bswap32(uint32_t x) {
    return (x >> 24) | ((x >> 8) & 0xFF00) | ((x << 8) & 0xFF0000) | (x << 24);
}

#define htons(x) bswap16(x)
#define ntohs(x) bswap16(x)
#define htonl(x) bswap32(x)
#define ntohl(x) bswap32(x)

// Bitmap operations
static inline void bitmap_set(uint64_t* bm, uint64_t bit) {
    bm[bit / 64] |= (1ULL << (bit % 64));
}

static inline void bitmap_clear(uint64_t* bm, uint64_t bit) {
    bm[bit / 64] &= ~(1ULL << (bit % 64));
}

static inline bool bitmap_test(uint64_t* bm, uint64_t bit) {
    return (bm[bit / 64] >> (bit % 64)) & 1;
}

// Spinlock operations
static inline void spin_init(spinlock_t* lock) {
    lock->lock = 0;
}

static inline void spin_lock(spinlock_t* lock) {
    while (__sync_lock_test_and_set(&lock->lock, 1)) {
        while (lock->lock) pause();
    }
}

static inline void spin_unlock(spinlock_t* lock) {
    __sync_lock_release(&lock->lock);
}

// Console
void kprintf(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
void kvprintf(const char* fmt, va_list args);
void kernel_panic(const char* msg) __attribute__((noreturn));
void console_early_init(void);
void console_set_framebuffer(void* fb, uint32_t w, uint32_t h, uint32_t pitch, uint32_t bpp);

// Memory
void pmm_init(uint64_t mb_info);
void* pmm_alloc_page(void);
void* pmm_alloc_pages(size_t count);
void* pmm_alloc_huge_page(void);
void pmm_free_page(void* addr);
void pmm_free_pages(void* addr, size_t count);
void pmm_get_stats(uint64_t* total, uint64_t* used, uint64_t* free);
uint64_t pmm_get_free(void);
void vmm_init(void);
bool vmm_map_page(uint64_t* pml4, uint64_t virt, uint64_t phys, uint64_t flags);
bool vmm_map_huge_page(uint64_t* pml4, uint64_t virt, uint64_t phys, uint64_t flags);
bool vmm_map_pages(uint64_t* pml4, void* virt, size_t count, uint64_t flags);
void vmm_unmap_page(uint64_t* pml4, uint64_t virt);
uint64_t vmm_create_address_space(void);
void vmm_switch_pml4(uint64_t pml4_phys);
void* vmm_get_phys(uint64_t* pml4, uint64_t virt);

// GDT/TSS
void gdt64_init(void);
void init_tss(void);
void tss_set_rsp0(uint64_t rsp);

// IDT
void idt_init(void);
void idt_set_gate(uint8_t vector, void* handler, uint8_t type);
void irq_register_handler(uint8_t irq, void (*handler)(void), void* context);
void interrupt_handler(uint64_t* frame, uint64_t num, uint64_t err);

// ACPI
bool acpi_init(uint8_t* rsdp);
void acpi_shutdown(void);
void acpi_reboot(void);
uint32_t acpi_get_cpu_count(void);
uint8_t acpi_get_cpu_apic_id(uint32_t idx);
uint32_t acpi_get_ioapic_count(void);
uint32_t acpi_get_ioapic_addr(uint32_t idx);
uint32_t acpi_get_ioapic_gsi_base(uint32_t idx);
bool acpi_initialized(void);
uint8_t acpi_get_version_major(void);
uint8_t acpi_get_version_minor(void);

// LAPIC
void lapic_init(void);
void lapic_init_ap(void);
uint32_t lapic_get_id(void);
void lapic_eoi(void);
void lapic_send_ipi(uint8_t apic_id, uint8_t vector);
void lapic_timer_calibrate(void);
uint64_t lapic_get_timer_ticks(void);
uint64_t lapic_get_timer_freq(void);
void lapic_timer_set_handler(void (*handler)(void));
void lapic_timer_start_periodic(uint64_t ms);

// IOAPIC
void ioapic_init(void);
void ioapic_set_irq(uint8_t irq, uint8_t vector, uint32_t dest);
void ioapic_mask_irq(uint8_t irq);
void ioapic_unmask_irq(uint8_t irq);

// SMP
void cpu_init_early(void);
struct cpu* cpu_get_current(void);
void smp_init(void);
uint32_t smp_get_cpu_count(void);
void ap_main(void);
void scheduler_ap_entry(void);

// Scheduler
void scheduler_init(void);
void schedule(void);
void yield(void);
void sleep(uint64_t ms);
struct process* process_create(const char* name, void* entry);
void context_switch(uint64_t* old_rsp, uint64_t new_rsp, uint64_t new_cr3);

// PCI
void pci_init(void);
struct pci_device* pci_find_class(uint32_t class_code);
struct pci_device* pci_find_device(uint16_t vendor, uint16_t device);
struct pci_device* pci_get_device(size_t idx);
size_t pci_get_device_count(void);
uint32_t pci_read_dword(struct pci_device* dev, uint8_t offset);
uint16_t pci_read_word(struct pci_device* dev, uint8_t offset);
void pci_write_word(struct pci_device* dev, uint8_t offset, uint16_t val);
uint64_t pci_read_bar(struct pci_device* dev, uint8_t bar);
void pci_enable_bus_mastering(struct pci_device* dev);
bool pci_enable_msi(struct pci_device* dev, uint8_t vector);

// Storage
bool ahci_init(void);
bool ahci_read_sectors(uint8_t port, uint64_t lba, uint32_t count, void* buf);
bool ahci_write_sectors(uint8_t port, uint64_t lba, uint32_t count, const void* buf);
uint8_t ahci_get_port_count(void);
bool nvme_init(void);
uint64_t nvme_get_boot_size(void);
bool nvme_read_boot(uint64_t lba, uint32_t count, void* buf);
bool nvme_write_boot(uint64_t lba, uint32_t count, const void* buf);

// USB
bool xhci_init(void);
void xhci_poll(void);

// Audio
bool hda_init(void);
void hda_beep(uint16_t freq, uint16_t duration_ms);
void hda_send_verb(uint8_t codec, uint8_t nid, uint32_t verb, uint8_t payload);

// Network
bool e1000e_init(void);
bool r8168_init(void);
void network_poll(void);
void r8168_poll(void);

// Laptop
void laptop_ec_init(void);
uint8_t laptop_get_cpu_temp(void);
uint16_t laptop_get_fan_rpm(void);
void laptop_set_fan_speed(uint8_t percent);
void laptop_thermal_init(void);
void laptop_thermal_poll(void);

// GPU
bool amdgpu_init(void);
void* amdgpu_get_framebuffer(void);
uint32_t amdgpu_get_fb_width(void);
uint32_t amdgpu_get_fb_height(void);
uint32_t amdgpu_get_fb_pitch(void);
uint32_t amdgpu_get_fb_bpp(void);
void amdgpu_putpixel(uint32_t x, uint32_t y, uint32_t color);
void amdgpu_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
void amdgpu_clear(uint32_t color);

// PIT
void pit_wait(uint32_t ms);

// Ext2 filesystem
bool ext2_mount(bool (*read_fn)(uint64_t, uint32_t, void*), bool (*write_fn)(uint64_t, uint32_t, const void*), uint64_t start_lba);
bool ext2_open(const char* path, void* file);
uint32_t ext2_read(void* file, void* buf, uint32_t size);
bool ext2_exists(const char* path);

// Ext2 file structure
struct ext2_inode {
    uint16_t i_mode;
    uint16_t i_uid;
    uint32_t i_size;
    uint32_t i_atime;
    uint32_t i_ctime;
    uint32_t i_mtime;
    uint32_t i_dtime;
    uint16_t i_gid;
    uint16_t i_links_count;
    uint32_t i_blocks;
    uint32_t i_flags;
    uint32_t i_osd1;
    uint32_t i_block[15];
    uint32_t i_generation;
    uint32_t i_file_acl;
    uint32_t i_dir_acl;
    uint32_t i_faddr;
    uint8_t  i_osd2[12];
};

struct ext2_file {
    struct ext2_inode inode;
    uint32_t pos;
    uint32_t size;
    uint8_t  type;
    char     name[256];
};

// Network stack
void network_init(void);
void network_rx(void* packet, uint16_t len);
void tcp_init(void);
void tcp_timer_tick(void);

// String functions (in string.c)
void* memset(void* s, int c, size_t n);
void* memcpy(void* d, const void* s, size_t n);
void* memmove(void* d, const void* s, size_t n);
int memcmp(const void* s1, const void* s2, size_t n);
void* memchr(const void* s, int c, size_t n);
size_t strlen(const char* s);
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, size_t n);
char* strcpy(char* d, const char* s);
char* strncpy(char* d, const char* s, size_t n);
char* strcat(char* d, const char* s);
char* strncat(char* d, const char* s, size_t n);
char* strchr(const char* s, int c);
char* strstr(const char* haystack, const char* needle);
size_t strspn(const char* s, const char* accept);
size_t strcspn(const char* s, const char* reject);

// kmalloc
void kmalloc_init(void);
void* kmalloc(size_t size);
void* kzalloc(size_t size);
void kfree(void* ptr);

// Realtek ALC audio
bool realtek_alc_init(uint8_t codec_addr, uint16_t vendor_id, uint16_t device_id);
void realtek_alc_poll(void);
void realtek_beep(uint16_t freq, uint16_t duration_ms);

// ============================================================================
// Network Stack Definitions (Critical for Ethernet RJ45)
// ============================================================================

// Ethernet Frame
struct eth_frame {
    uint8_t  dst[6];
    uint8_t  src[6];
    uint16_t type;
} __attribute__((packed));

#define ETH_TYPE_IPV4    0x0800
#define ETH_TYPE_ARP    0x0806
#define ETH_TYPE_IPV6    0x86DD

// IPv4 Header
struct ipv4_header {
    uint8_t  version_ihl;
    uint8_t  tos;
    uint16_t total_len;
    uint16_t id;
    uint16_t flags_frag;
    uint8_t  ttl;
    uint8_t  protocol;
    uint16_t checksum;
    uint32_t src_ip;
    uint32_t dst_ip;
} __attribute__((packed));

#define IP_PROTO_ICMP   1
#define IP_PROTO_TCP    6
#define IP_PROTO_UDP    17

// ARP Packet
struct arp_packet {
    uint16_t hw_type;
    uint16_t proto_type;
    uint8_t  hw_size;
    uint8_t  proto_size;
    uint16_t opcode;
    uint8_t  sender_mac[6];
    uint32_t sender_ip;
    uint8_t  target_mac[6];
    uint32_t target_ip;
} __attribute__((packed));

#define ARP_OP_REQUEST  1
#define ARP_OP_REPLY    2
#define ARP_HW_ETHERNET 1
#define ARP_PROTO_IP    0x0800

// TCP Header
struct tcp_header {
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t seq;
    uint32_t ack;
    uint8_t  data_off;
    uint8_t  flags;
    uint16_t window;
    uint16_t checksum;
    uint16_t urgent;
} __attribute__((packed));

#define TCP_FLAG_FIN  0x01
#define TCP_FLAG_SYN  0x02
#define TCP_FLAG_RST  0x04
#define TCP_FLAG_PSH  0x08
#define TCP_FLAG_ACK  0x10
#define TCP_FLAG_URG  0x20

// UDP Header
struct udp_header {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;
    uint16_t checksum;
} __attribute__((packed));

// Network interface
struct netif {
    uint8_t  mac[6];
    uint32_t ip;
    uint32_t netmask;
    uint32_t gateway;
    bool     connected;
    void     (*send)(void* pkt, uint16_t len);
};

// Network buffer
struct net_buf {
    void*    data;
    uint16_t len;
    uint16_t capacity;
    struct net_buf* next;
};

// Network functions
void netif_init(uint8_t* mac, uint32_t ip, uint32_t netmask, uint32_t gateway);
void netif_set_send_callback(void (*fn)(void*, uint16_t));
void eth_rx(void* frame, uint16_t len);
void arp_rx(struct arp_packet* arp);
void ipv4_rx(struct ipv4_header* ip, void* payload, uint16_t len);
void tcp_rx(struct tcp_header* tcp, void* payload, uint16_t len);
void udp_rx(struct udp_header* udp, void* payload, uint16_t len);
uint16_t ip_checksum(void* data, uint32_t len);
uint16_t tcp_checksum(struct ipv4_header* ip, struct tcp_header* tcp, void* payload, uint16_t len);
uint16_t udp_checksum(struct ipv4_header* ip, struct udp_header* udp, void* payload, uint16_t len);

// ============================================================================
// PCIe/ECAM Configuration (Required for modern PCI access)
// ============================================================================

#define PCIE_ECAM_ADDR(bus, dev, func, reg) \
    (0xFFFFFFFF80000000ULL | ((uint64_t)(bus) << 20) | ((uint64_t)(dev) << 15) | \
     ((uint64_t)(func) << 12) | ((reg) & 0xFFF))

#define PCIE_CONFIG_ADDRESS(bdf, offset) \
    (0xFFFFFFFF80000000ULL | ((uint64_t)(bdf) << 12) | (offset))

// ============================================================================
// CPU Feature Flags (CR4 bits for Ryzen/AMD-V)
// ============================================================================

#define CR4_PAE        0x00000020
#define CR4_PGE        0x00000080
#define CR4_PCIDE      0x00020000
#define CR4_SMEP       0x00100000
#define CR4_SMAP       0x00200000
#define CR4_OSXSAVE    0x00040000

// EFER MSR bits
#define EFER_MSR       0xC0000080
#define EFER_NXE       (1ULL << 11)
#define EFER_LME       (1ULL << 8)
#define EFER_LMA       (1ULL << 10)
#define EFER_SCE       (1ULL << 0)

// ============================================================================
// Additional Critical Structures
// ============================================================================

// ACPI RSDP Structure
struct rsdp_descriptor {
    char     signature[8];
    uint8_t  checksum;
    char     oem_id[6];
    uint8_t  revision;
    uint32_t rsdt_address;
    uint32_t length;
    uint64_t xsdt_address;
    uint8_t  extended_checksum;
    char     reserved[3];
} __attribute__((packed));

// ACPI MADT Structure
struct madt_header {
    char     signature[4];
    uint32_t length;
    uint8_t  revision;
    uint8_t  checksum;
    char     oem_id[6];
    char     oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
    uint32_t local_apic_addr;
    uint32_t flags;
} __attribute__((packed));

// MADT Local APIC Entry
struct madt_lapic {
    uint8_t  type;
    uint8_t  length;
    uint8_t  acpi_id;
    uint8_t  apic_id;
    uint32_t flags;
} __attribute__((packed));

// MADT IO APIC Entry
struct madt_ioapic {
    uint8_t  type;
    uint8_t  length;
    uint8_t  ioapic_id;
    uint8_t  reserved;
    uint32_t addr;
    uint32_t gsi_base;
} __attribute__((packed));

// MSI Address Format
#define MSI_ADDRESS_BASE       0xFEE00000
#define MSI_ADDRESS(cfg)       (MSI_ADDRESS_BASE | ((cfg) << 12))
#define MSI_DATA_VECTOR(vec)   (vec & 0xFF)
#define MSI_DATA_TRIGGER_EDGE  (0 << 14)
#define MSI_DATA_TRIGGER_LEVEL (1 << 14)
#define MSI_DATA_LEVEL_ASSERT  (1 << 15)
#define MSI_DATA_DEST_MODE_PHY (0 << 18)
#define MSI_DATA_DEST_MODE_LOG (1 << 18)
#define MSI_DATA_REDIR_FIXED   (0 << 19)

#endif
