// vmm.c — Virtual Memory Manager with 4-level paging FIXED
#include "kernel.h"

#define PML4_INDEX(v)   (((v) >> 39) & 0x1FF)
#define PDPT_INDEX(v)   (((v) >> 30) & 0x1FF)
#define PD_INDEX(v)     (((v) >> 21) & 0x1FF)
#define PT_INDEX(v)     (((v) >> 12) & 0x1FF)

#define RECURSIVE_INDEX 510

static uint64_t* kernel_pml4 = NULL;

void vmm_init(void) {
    // Allocate kernel PML4
    kernel_pml4 = (uint64_t*)pmm_alloc_page();
    if (!kernel_pml4) kernel_panic("VMM: Failed to allocate PML4");
    memset(kernel_pml4, 0, PAGE_SIZE);
    
    // Identity map first 1GB using huge pages
    for (uint64_t addr = 0; addr < 0x40000000; addr += HUGE_PAGE_SIZE) {
        vmm_map_huge_page(kernel_pml4, addr, addr, PT_PRESENT | PT_WRITABLE | PT_HUGE);
    }
    
    // Map kernel higher half
    uint64_t kernel_size = VIRT_TO_PHYS((uint64_t)_kernel_end) - 0x100000;
    for (uint64_t off = 0; off < kernel_size; off += PAGE_SIZE) {
        vmm_map_page(kernel_pml4,
            KERNEL_TEXT_START + off,
            0x100000 + off,
            PT_PRESENT | PT_WRITABLE | PT_GLOBAL);
    }
    
    // Map VGA text mode memory
    vmm_map_page(kernel_pml4, 0xFFFFFFFF800B8000ULL, 0xB8000,
        PT_PRESENT | PT_WRITABLE | PT_GLOBAL);
    
    // Recursive mapping for page table access
    kernel_pml4[RECURSIVE_INDEX] = VIRT_TO_PHYS((uint64_t)kernel_pml4) |
        PT_PRESENT | PT_WRITABLE;
    
    // Switch to new page tables
    write_cr3(VIRT_TO_PHYS((uint64_t)kernel_pml4));
    
    kprintf("VMM: Kernel PML4 at %p, 4-level paging active\n", kernel_pml4);
}

static uint64_t* get_or_create_table(uint64_t* parent, uint16_t index, uint64_t flags) {
    if (parent[index] & PT_PRESENT) {
        return (uint64_t*)PHYS_TO_VIRT(parent[index] & PAGE_MASK);
    }
    
    uint64_t* table = (uint64_t*)pmm_alloc_page();
    if (!table) return NULL;
    
    memset(table, 0, PAGE_SIZE);
    parent[index] = VIRT_TO_PHYS((uint64_t)table) | flags;
    
    return table;
}

bool vmm_map_page(uint64_t* pml4, uint64_t virt, uint64_t phys, uint64_t flags) {
    uint16_t pml4_idx = PML4_INDEX(virt);
    uint16_t pdpt_idx = PDPT_INDEX(virt);
    uint16_t pd_idx = PD_INDEX(virt);
    uint16_t pt_idx = PT_INDEX(virt);
    
    uint64_t* pdpt = get_or_create_table(pml4, pml4_idx, PT_PRESENT | PT_WRITABLE);
    if (!pdpt) return false;
    
    uint64_t* pd = get_or_create_table(pdpt, pdpt_idx, PT_PRESENT | PT_WRITABLE);
    if (!pd) return false;
    
    uint64_t* pt = get_or_create_table(pd, pd_idx, PT_PRESENT | PT_WRITABLE);
    if (!pt) return false;
    
    // Unmap existing if present
    if (pt[pt_idx] & PT_PRESENT) {
        pt[pt_idx] = 0;
        invlpg(virt);
    }
    
    pt[pt_idx] = (phys & PAGE_MASK) | flags;
    invlpg(virt);
    
    return true;
}

bool vmm_map_huge_page(uint64_t* pml4, uint64_t virt, uint64_t phys, uint64_t flags) {
    uint16_t pml4_idx = PML4_INDEX(virt);
    uint16_t pdpt_idx = PDPT_INDEX(virt);
    uint16_t pd_idx = PD_INDEX(virt);
    
    uint64_t* pdpt = get_or_create_table(pml4, pml4_idx, PT_PRESENT | PT_WRITABLE);
    if (!pdpt) return false;
    
    uint64_t* pd = get_or_create_table(pdpt, pdpt_idx, PT_PRESENT | PT_WRITABLE);
    if (!pd) return false;
    
    if (pd[pd_idx] & PT_PRESENT) {
        pd[pd_idx] = 0;
        invlpg(virt);
    }
    
    pd[pd_idx] = (phys & ~(HUGE_PAGE_SIZE - 1)) | flags | PT_HUGE;
    invlpg(virt);
    
    return true;
}

bool vmm_map_pages(uint64_t* pml4, void* virt, size_t count, uint64_t flags) {
    uint64_t v = (uint64_t)virt;
    uint64_t p = 0; // Will be allocated from PMM
    
    for (size_t i = 0; i < count; i++) {
        void* page = pmm_alloc_page();
        if (!page) {
            // Rollback on failure
            for (size_t j = 0; j < i; j++) {
                vmm_unmap_page(pml4, v + j * PAGE_SIZE);
            }
            return false;
        }
        p = VIRT_TO_PHYS(page);
        if (!vmm_map_page(pml4, v + i * PAGE_SIZE, p, flags)) {
            return false;
        }
    }
    return true;
}

void vmm_unmap_page(uint64_t* pml4, uint64_t virt) {
    uint16_t pml4_idx = PML4_INDEX(virt);
    if (!(pml4[pml4_idx] & PT_PRESENT)) return;
    
    uint64_t* pdpt = (uint64_t*)PHYS_TO_VIRT(pml4[pml4_idx] & PAGE_MASK);
    uint16_t pdpt_idx = PDPT_INDEX(virt);
    if (!(pdpt[pdpt_idx] & PT_PRESENT)) return;
    
    uint64_t* pd = (uint64_t*)PHYS_TO_VIRT(pdpt[pdpt_idx] & PAGE_MASK);
    uint16_t pd_idx = PD_INDEX(virt);
    if (!(pd[pd_idx] & PT_PRESENT)) return;
    
    if (pd[pd_idx] & PT_HUGE) {
        pd[pd_idx] = 0;
        invlpg(virt);
        return;
    }
    
    uint64_t* pt = (uint64_t*)PHYS_TO_VIRT(pd[pd_idx] & PAGE_MASK);
    uint16_t pt_idx = PT_INDEX(virt);
    pt[pt_idx] = 0;
    invlpg(virt);
}

uint64_t vmm_create_address_space(void) {
    uint64_t* pml4 = (uint64_t*)pmm_alloc_page();
    if (!pml4) return 0;
    
    memset(pml4, 0, PAGE_SIZE);
    
    // Copy kernel mappings (indices 256-511 for higher half)
    for (int i = 256; i < 512; i++) {
        if (kernel_pml4[i] & PT_PRESENT) {
            pml4[i] = kernel_pml4[i];
        }
    }
    
    // Recursive mapping
    pml4[RECURSIVE_INDEX] = VIRT_TO_PHYS((uint64_t)pml4) | PT_PRESENT | PT_WRITABLE;
    
    return VIRT_TO_PHYS((uint64_t)pml4);
}

void vmm_switch_pml4(uint64_t pml4_phys) {
    write_cr3(pml4_phys);
}

void* vmm_get_phys(uint64_t* pml4, uint64_t virt) {
    uint16_t pml4_idx = PML4_INDEX(virt);
    if (!(pml4[pml4_idx] & PT_PRESENT)) return NULL;
    
    uint64_t* pdpt = (uint64_t*)PHYS_TO_VIRT(pml4[pml4_idx] & PAGE_MASK);
    uint16_t pdpt_idx = PDPT_INDEX(virt);
    if (!(pdpt[pdpt_idx] & PT_PRESENT)) return NULL;
    
    uint64_t* pd = (uint64_t*)PHYS_TO_VIRT(pdpt[pdpt_idx] & PAGE_MASK);
    uint16_t pd_idx = PD_INDEX(virt);
    if (!(pd[pd_idx] & PT_PRESENT)) return NULL;
    
    if (pd[pd_idx] & PT_HUGE) {
        return (void*)((pd[pd_idx] & ~(HUGE_PAGE_SIZE - 1)) | (virt & (HUGE_PAGE_SIZE - 1)));
    }
    
    uint64_t* pt = (uint64_t*)PHYS_TO_VIRT(pd[pd_idx] & PAGE_MASK);
    uint16_t pt_idx = PT_INDEX(virt);
    if (!(pt[pt_idx] & PT_PRESENT)) return NULL;
    
    return (void*)((pt[pt_idx] & PAGE_MASK) | (virt & (PAGE_SIZE - 1)));
}
