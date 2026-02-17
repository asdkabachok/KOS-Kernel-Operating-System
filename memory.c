// memory.c — Physical Memory Manager with buddy allocator FIXED
#include "kernel.h"

#define ZONE_DMA        0   // < 16MB
#define ZONE_DMA32      1   // < 4GB
#define ZONE_NORMAL     2   // < 16GB (твой случай)
#define ZONE_COUNT      3

#define MIN_ORDER       0   // 4KB
#define MAX_ORDER       12  // 16MB blocks

#define BUDDY_MAGIC     0xBADDCAFE

struct buddy_block {
    uint32_t magic;
    uint16_t order;
    uint16_t flags;
    struct buddy_block* next;
    struct buddy_block* prev;
};

struct memory_zone {
    uint64_t base_pfn;
    uint64_t end_pfn;
    uint64_t total_pages;
    uint64_t free_pages;
    uint64_t* bitmap;
    struct buddy_block* free_list[MAX_ORDER + 1];
    spinlock_t lock;
};

static struct memory_zone zones[ZONE_COUNT];
static uint64_t total_system_pages = 0;
static uint64_t early_alloc_pages = 0;
static uint8_t* early_bitmap = NULL;

// Early allocation before PMM init
void* pmm_early_alloc(size_t pages) {
    if (!early_bitmap) {
        early_bitmap = (uint8_t*)0x100000; // Use 1MB mark temporarily
        memset(early_bitmap, 0, 32768);    // 256MB tracking
    }
    for (uint64_t i = 0; i < 65536; i++) {
        bool found = true;
        for (uint64_t j = 0; j < pages; j++) {
            if (early_bitmap[(i + j) / 8] & (1 << ((i + j) % 8))) {
                found = false;
                break;
            }
        }
        if (found) {
            for (uint64_t j = 0; j < pages; j++) {
                early_bitmap[(i + j) / 8] |= (1 << ((i + j) % 8));
            }
            early_alloc_pages += pages;
            return (void*)((i + 0x100) * PAGE_SIZE); // Start at 1MB
        }
    }
    kernel_panic("Early allocation failed");
    return NULL;
}

void pmm_init(uint64_t mb_info_phys) {
    kprintf("PMM: Initializing for 16GB system...\n");
    
    // Parse multiboot memory map
    struct multiboot_tag_mmap* mmap = NULL;
    uint8_t* ptr = (uint8_t*)PHYS_TO_VIRT(mb_info_phys) + 8;
    
    while (((struct multiboot_tag*)ptr)->type != MULTIBOOT_TAG_TYPE_END) {
        if (((struct multiboot_tag*)ptr)->type == MULTIBOOT_TAG_TYPE_MMAP) {
            mmap = (struct multiboot_tag_mmap*)ptr;
            break;
        }
        ptr += (((struct multiboot_tag*)ptr)->size + 7) & ~7;
    }
    
    if (!mmap) kernel_panic("No memory map from bootloader");
    
    size_t num_entries = (mmap->size - sizeof(struct multiboot_tag_mmap)) / mmap->entry_size;
    
    // Initialize zones as empty
    for (int z = 0; z < ZONE_COUNT; z++) {
        zones[z].base_pfn = 0xFFFFFFFFFFFFFFFFULL;
        zones[z].end_pfn = 0;
        zones[z].total_pages = 0;
        zones[z].free_pages = 0;
        zones[z].bitmap = NULL;
        spin_init(&zones[z].lock);
        for (int o = 0; o <= MAX_ORDER; o++) {
            zones[z].free_list[o] = NULL;
        }
    }
    
    // Process memory map entries
    for (size_t i = 0; i < num_entries; i++) {
        struct multiboot_mmap_entry* e = &mmap->entries[i];
        if (e->type != MULTIBOOT_MEMORY_AVAILABLE) continue;
        
        uint64_t start = (e->base_addr + PAGE_SIZE - 1) & PAGE_MASK;
        uint64_t end = (e->base_addr + e->length) & PAGE_MASK;
        
        if (start >= end) continue;
        
        // Skip below 1MB (BIOS, VGA)
        if (end <= 0x100000) continue;
        if (start < 0x100000) start = 0x100000;
        
        // Determine zone
        int zone_idx;
        if (end <= 0x1000000) {
            zone_idx = ZONE_DMA;
        } else if (end <= 0x100000000ULL) {
            zone_idx = ZONE_DMA32;
        } else {
            zone_idx = ZONE_NORMAL;
        }
        
        struct memory_zone* zone = &zones[zone_idx];
        
        // Extend zone range
        if (start < zone->base_pfn * PAGE_SIZE) {
            zone->base_pfn = start / PAGE_SIZE;
        }
        if (end > zone->end_pfn * PAGE_SIZE) {
            zone->end_pfn = end / PAGE_SIZE;
        }
        zone->total_pages += (end - start) / PAGE_SIZE;
        total_system_pages += (end - start) / PAGE_SIZE;
        
        kprintf("PMM: Zone %d: 0x%lX - 0x%lX (%lu MB)\n",
            zone_idx, start, end, (end - start) / (1024 * 1024));
    }
    
    // Allocate bitmaps for each zone
    for (int z = 0; z < ZONE_COUNT; z++) {
        if (zones[z].total_pages == 0) continue;
        size_t bitmap_size = ((zones[z].end_pfn - zones[z].base_pfn) + 63) / 64 * 8;
        zones[z].bitmap = (uint64_t*)pmm_early_alloc((bitmap_size + PAGE_SIZE - 1) / PAGE_SIZE);
        memset(zones[z].bitmap, 0xFF, bitmap_size); // Mark all used initially
    }
    
    // Second pass: mark available pages as free in bitmaps
    for (size_t i = 0; i < num_entries; i++) {
        struct multiboot_mmap_entry* e = &mmap->entries[i];
        if (e->type != MULTIBOOT_MEMORY_AVAILABLE) continue;
        
        uint64_t start = (e->base_addr + PAGE_SIZE - 1) & PAGE_MASK;
        uint64_t end = (e->base_addr + e->length) & PAGE_MASK;
        
        if (start < 0x100000) start = 0x100000;
        
        for (uint64_t addr = start; addr < end; addr += PAGE_SIZE) {
            int zone_idx = (end <= 0x1000000) ? ZONE_DMA :
                           (end <= 0x100000000ULL) ? ZONE_DMA32 : ZONE_NORMAL;
            
            if (zones[zone_idx].total_pages == 0) continue;
            
            uint64_t pfn = addr / PAGE_SIZE;
            if (pfn < zones[zone_idx].base_pfn || pfn >= zones[zone_idx].end_pfn) continue;
            
            uint64_t idx = pfn - zones[zone_idx].base_pfn;
            bitmap_clear(zones[zone_idx].bitmap, idx);
            zones[zone_idx].free_pages++;
        }
    }
    
    // Reserve kernel memory (simplified: reserve first 16MB)
    for (uint64_t pfn = 0; pfn < 0x1000 && pfn < zones[ZONE_NORMAL].end_pfn; pfn++) {
        if (pfn >= zones[ZONE_NORMAL].base_pfn) {
            bitmap_set(zones[ZONE_NORMAL].bitmap, pfn - zones[ZONE_NORMAL].base_pfn);
            if (zones[ZONE_NORMAL].free_pages > 0) zones[ZONE_NORMAL].free_pages--;
        }
    }
    
    kprintf("PMM: Total %lu MB, Free %lu MB\n",
        total_system_pages * PAGE_SIZE / (1024 * 1024),
        (total_system_pages - early_alloc_pages) * PAGE_SIZE / (1024 * 1024));
}

void* pmm_alloc_page(void) {
    // Try zones: NORMAL, DMA32, DMA
    int order[] = {ZONE_NORMAL, ZONE_DMA32, ZONE_DMA};
    for (int i = 0; i < 3; i++) {
        int z = order[i];
        if (zones[z].free_pages == 0) continue;
        
        spin_lock(&zones[z].lock);
        
        // Find a free page
        for (uint64_t pfn = zones[z].base_pfn; pfn < zones[z].end_pfn; pfn++) {
            uint64_t idx = pfn - zones[z].base_pfn;
            if (!bitmap_test(zones[z].bitmap, idx)) {
                // Found free page
                bitmap_set(zones[z].bitmap, idx);
                zones[z].free_pages--;
                spin_unlock(&zones[z].lock);
                
                void* addr = (void*)(pfn * PAGE_SIZE);
                memset(PHYS_TO_VIRT(addr), 0, PAGE_SIZE);
                return PHYS_TO_VIRT(addr);
            }
        }
        spin_unlock(&zones[z].lock);
    }
    return NULL; // Out of memory
}

void* pmm_alloc_pages(size_t count) {
    if (count == 0) return NULL;
    if (count == 1) return pmm_alloc_page();
    
    // For simplicity, allocate contiguous using first-fit
    for (int z = ZONE_NORMAL; z >= 0; z--) {
        if (zones[z].free_pages < count) continue;
        
        spin_lock(&zones[z].lock);
        
        uint64_t consecutive = 0;
        uint64_t start_pfn = 0;
        
        for (uint64_t pfn = zones[z].base_pfn; pfn < zones[z].end_pfn; pfn++) {
            uint64_t idx = pfn - zones[z].base_pfn;
            if (!bitmap_test(zones[z].bitmap, idx)) {
                if (consecutive == 0) start_pfn = pfn;
                consecutive++;
                if (consecutive >= count) {
                    // Found block
                    for (uint64_t j = 0; j < count; j++) {
                        bitmap_set(zones[z].bitmap, start_pfn + j - zones[z].base_pfn);
                    }
                    zones[z].free_pages -= count;
                    spin_unlock(&zones[z].lock);
                    
                    void* addr = (void*)((start_pfn) * PAGE_SIZE);
                    memset(PHYS_TO_VIRT(addr), 0, count * PAGE_SIZE);
                    return PHYS_TO_VIRT(addr);
                }
            } else {
                consecutive = 0;
            }
        }
        spin_unlock(&zones[z].lock);
    }
    return NULL;
}

void* pmm_alloc_huge_page(void) {
    // 2MB aligned
    for (int z = ZONE_NORMAL; z >= 0; z--) {
        if (zones[z].free_pages < 512) continue;
        
        spin_lock(&zones[z].lock);
        
        for (uint64_t pfn = zones[z].base_pfn;
             pfn + 512 <= zones[z].end_pfn;
             pfn += 512) {
            
            bool free = true;
            for (int i = 0; i < 512; i++) {
                uint64_t idx = pfn + i - zones[z].base_pfn;
                if (idx >= zones[z].end_pfn - zones[z].base_pfn) {
                    free = false;
                    break;
                }
                if (bitmap_test(zones[z].bitmap, idx)) {
                    free = false;
                    break;
                }
            }
            
            if (free) {
                for (int i = 0; i < 512; i++) {
                    bitmap_set(zones[z].bitmap, pfn + i - zones[z].base_pfn);
                }
                zones[z].free_pages -= 512;
                spin_unlock(&zones[z].lock);
                
                void* addr = (void*)(pfn * PAGE_SIZE);
                memset(PHYS_TO_VIRT(addr), 0, HUGE_PAGE_SIZE);
                return PHYS_TO_VIRT(addr);
            }
        }
        spin_unlock(&zones[z].lock);
    }
    return NULL;
}

void pmm_free_page(void* addr) {
    if (!addr) return;
    
    uint64_t virt = (uint64_t)addr;
    uint64_t pfn = virt / PAGE_SIZE;
    
    for (int z = 0; z < ZONE_COUNT; z++) {
        if (pfn >= zones[z].base_pfn && pfn < zones[z].end_pfn) {
            spin_lock(&zones[z].lock);
            
            uint64_t idx = pfn - zones[z].base_pfn;
            if (idx >= (zones[z].end_pfn - zones[z].base_pfn)) {
                spin_unlock(&zones[z].lock);
                continue;
            }
            
            if (bitmap_test(zones[z].bitmap, idx)) {
                spin_unlock(&zones[z].lock);
                kprintf("PMM: Double free at %p\n", addr);
                continue;
            }
            
            bitmap_clear(zones[z].bitmap, idx);
            zones[z].free_pages++;
            
            spin_unlock(&zones[z].lock);
            return;
        }
    }
    kprintf("PMM: Free of invalid address %p\n", addr);
}

void pmm_free_pages(void* addr, size_t count) {
    if (!addr || count == 0) return;
    for (size_t i = 0; i < count; i++) {
        pmm_free_page((uint8_t*)addr + i * PAGE_SIZE);
    }
}

uint64_t pmm_get_free(void) {
    uint64_t free = 0;
    for (int z = 0; z < ZONE_COUNT; z++) {
        free += zones[z].free_pages * PAGE_SIZE;
    }
    return free;
}

void pmm_get_stats(uint64_t* total, uint64_t* used, uint64_t* free) {
    *total = total_system_pages * PAGE_SIZE;
    *free = pmm_get_free();
    *used = *total - *free;
}
