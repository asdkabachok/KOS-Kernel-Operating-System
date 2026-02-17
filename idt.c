// idt.c — Interrupt Descriptor Table FIXED
#include "kernel.h"

#define IDT_ENTRIES     256

struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

static struct idt_entry idt[IDT_ENTRIES];
static struct idt_ptr idtp;
static void (*irq_handlers[16])(void) = {0};
static void* irq_contexts[16] = {0};

// Exception names for debugging
static const char* exception_names[32] = {
    "Division Error", "Debug", "NMI", "Breakpoint",
    "Overflow", "BOUND Range Exceeded", "Invalid Opcode",
    "Device Not Available", "Double Fault", "Coprocessor Segment Overrun",
    "Invalid TSS", "Segment Not Present", "Stack Fault",
    "General Protection", "Page Fault", "Reserved",
    "x87 FPU Error", "Alignment Check", "Machine Check", "SIMD FP",
    "Virtualization", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Security Exception", "Reserved"
};

extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr21(void);
extern void isr22(void);
extern void isr23(void);
extern void isr24(void);
extern void isr25(void);
extern void isr26(void);
extern void isr27(void);
extern void isr28(void);
extern void isr29(void);
extern void isr30(void);
extern void isr31(void);

extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);
extern void irq5(void);
extern void irq6(void);
extern void irq7(void);
extern void irq8(void);
extern void irq9(void);
extern void irq10(void);
extern void irq11(void);
extern void irq12(void);
extern void irq13(void);
extern void irq14(void);
extern void irq15(void);

static void (*exception_handlers[32])(uint64_t, uint64_t) = {0};

void idt_set_gate(uint8_t num, void* handler, uint8_t type_attr) {
    uint64_t addr = (uint64_t)handler;
    
    idt[num].offset_low = addr & 0xFFFF;
    idt[num].offset_mid = (addr >> 16) & 0xFFFF;
    idt[num].offset_high = addr >> 32;
    idt[num].selector = 0x08;  // Kernel code segment
    idt[num].ist = 0;
    idt[num].type_attr = type_attr;
    idt[num].zero = 0;
}

void idt_init(void) {
    memset(idt, 0, sizeof(idt));
    
    // Set all exception handlers (0-31)
    idt_set_gate(0, (void*)isr0, 0x8E);
    idt_set_gate(1, (void*)isr1, 0x8E);
    idt_set_gate(2, (void*)isr2, 0x8E);
    idt_set_gate(3, (void*)isr3, 0x8E);
    idt_set_gate(4, (void*)isr4, 0x8E);
    idt_set_gate(5, (void*)isr5, 0x8E);
    idt_set_gate(6, (void*)isr6, 0x8E);
    idt_set_gate(7, (void*)isr7, 0x8E);
    idt_set_gate(8, (void*)isr8, 0x8E);
    idt_set_gate(9, (void*)isr9, 0x8E);
    idt_set_gate(10, (void*)isr10, 0x8E);
    idt_set_gate(11, (void*)isr11, 0x8E);
    idt_set_gate(12, (void*)isr12, 0x8E);
    idt_set_gate(13, (void*)isr13, 0x8E);
    idt_set_gate(14, (void*)isr14, 0x8E);
    idt_set_gate(15, (void*)isr15, 0x8E);
    idt_set_gate(16, (void*)isr16, 0x8E);
    idt_set_gate(17, (void*)isr17, 0x8E);
    idt_set_gate(18, (void*)isr18, 0x8E);
    idt_set_gate(19, (void*)isr19, 0x8E);
    idt_set_gate(20, (void*)isr20, 0x8E);
    idt_set_gate(21, (void*)isr21, 0x8E);
    idt_set_gate(22, (void*)isr22, 0x8E);
    idt_set_gate(23, (void*)isr23, 0x8E);
    idt_set_gate(24, (void*)isr24, 0x8E);
    idt_set_gate(25, (void*)isr25, 0x8E);
    idt_set_gate(26, (void*)isr26, 0x8E);
    idt_set_gate(27, (void*)isr27, 0x8E);
    idt_set_gate(28, (void*)isr28, 0x8E);
    idt_set_gate(29, (void*)isr29, 0x8E);
    idt_set_gate(30, (void*)isr30, 0x8E);
    idt_set_gate(31, (void*)isr31, 0x8E);
    
    // IRQs 0-15 -> vectors 32-47
    idt_set_gate(32, (void*)irq0, 0x8E);
    idt_set_gate(33, (void*)irq1, 0x8E);
    idt_set_gate(34, (void*)irq2, 0x8E);
    idt_set_gate(35, (void*)irq3, 0x8E);
    idt_set_gate(36, (void*)irq4, 0x8E);
    idt_set_gate(37, (void*)irq5, 0x8E);
    idt_set_gate(38, (void*)irq6, 0x8E);
    idt_set_gate(39, (void*)irq7, 0x8E);
    idt_set_gate(40, (void*)irq8, 0x8E);
    idt_set_gate(41, (void*)irq9, 0x8E);
    idt_set_gate(42, (void*)irq10, 0x8E);
    idt_set_gate(43, (void*)irq11, 0x8E);
    idt_set_gate(44, (void*)irq12, 0x8E);
    idt_set_gate(45, (void*)irq13, 0x8E);
    idt_set_gate(46, (void*)irq14, 0x8E);
    idt_set_gate(47, (void*)irq15, 0x8E);
    
    // Load IDT
    idtp.limit = sizeof(idt) - 1;
    idtp.base = (uint64_t)idt;
    
    asm volatile ("lidt %0" : : "m"(idtp));
    
    kprintf("IDT: Initialized %d entries\n", IDT_ENTRIES);
}

void irq_register_handler(uint8_t irq, void (*handler)(void), void* context) {
    if (irq < 16) {
        irq_handlers[irq] = handler;
        irq_contexts[irq] = context;
    }
}

void interrupt_handler(uint64_t* frame, uint64_t num, uint64_t err) {
    (void)frame;
    
    if (num < 32) {
        // Exception
        kprintf("EXCEPTION %lu: %s\n", num, exception_names[num]);
        kprintf("  Error code: %lu\n", err);
        if (num == 14) { // Page fault
            kprintf("  CR2=%p\n", read_cr2());
            kprintf("  Present: %d, Write: %d, User: %d\n",
                (err & 1) ? 1 : 0, (err & 2) ? 1 : 0, (err & 4) ? 1 : 0);
        }
        kernel_panic("Unhandled exception");
    } else if (num >= 32 && num < 48) {
        // IRQ
        uint8_t irq = num - 32;
        if (irq_handlers[irq]) {
            irq_handlers[irq]();
        }
        lapic_eoi();
    }
}
