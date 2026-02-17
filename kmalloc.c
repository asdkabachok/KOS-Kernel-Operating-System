// kmalloc.c — Simple kernel heap using slab allocator FIXED
#include "kernel.h"

#define KMALLOC_MIN_SIZE    16
#define KMALLOC_MAX_SIZE   2048
#define KMALLOC_NUM_SIZES   8   // 16, 32, 64, 128, 256, 512, 1024, 2048

struct slab_header {
    struct slab_header* next;
    uint32_t magic;
    uint32_t size;
};

struct slab_cache {
    uint32_t size;
    struct slab_header* free_list;
    uint64_t allocations;
    uint64_t frees;
};

static struct slab_cache slabs[KMALLOC_NUM_SIZES];
static spinlock_t kmalloc_lock;
static bool kmalloc_initialized = false;

void kmalloc_init(void) {
    const uint32_t sizes[KMALLOC_NUM_SIZES] = {16, 32, 64, 128, 256, 512, 1024, 2048};
    
    for (int i = 0; i < KMALLOC_NUM_SIZES; i++) {
        slabs[i].size = sizes[i];
        slabs[i].free_list = NULL;
        slabs[i].allocations = 0;
        slabs[i].frees = 0;
    }
    
    spin_init(&kmalloc_lock);
    kmalloc_initialized = true;
    kprintf("kmalloc: Initialized\n");
}

static int size_to_index(size_t size) {
    for (int i = 0; i < KMALLOC_NUM_SIZES; i++) {
        if (size <= slabs[i].size) return i;
    }
    return -1;
}

void* kmalloc(size_t size) {
    if (!kmalloc_initialized) kmalloc_init();
    if (size == 0) return NULL;
    
    if (size > KMALLOC_MAX_SIZE) {
        // Large allocation: use direct page allocation
        size_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
        void* addr = pmm_alloc_pages(pages);
        if (addr) {
            memset(addr, 0, pages * PAGE_SIZE);
        }
        return addr;
    }
    
    int idx = size_to_index(size);
    if (idx < 0) return NULL;
    
    spin_lock(&kmalloc_lock);
    
    struct slab_cache* slab = &slabs[idx];
    
    if (slab->free_list) {
        struct slab_header* obj = slab->free_list;
        slab->free_list = obj->next;
        slab->allocations++;
        spin_unlock(&kmalloc_lock);
        
        obj->magic = 0xDEADBEEF;
        obj->size = slab->size;
        
        return (void*)(obj + 1);
    }
    
    // Allocate new page and split into objects
    void* page = pmm_alloc_page();
    if (!page) {
        spin_unlock(&kmalloc_lock);
        return NULL;
    }
    
    uint8_t* ptr = (uint8_t*)page;
    uint32_t num_objs = PAGE_SIZE / slab->size;
    
    // Build free list
    for (uint32_t i = 0; i < num_objs - 1; i++) {
        struct slab_header* h = (struct slab_header*)(ptr + i * slab->size);
        h->next = (struct slab_header*)(ptr + (i + 1) * slab->size);
        h->magic = 0;
        h->size = 0;
    }
    
    // Last one
    struct slab_header* last = (struct slab_header*)(ptr + (num_objs - 1) * slab->size);
    last->next = NULL;
    last->magic = 0;
    last->size = 0;
    
    slab->free_list = (struct slab_header*)ptr;
    
    // Return first object
    struct slab_header* obj = slab->free_list;
    slab->free_list = obj->next;
    slab->allocations++;
    
    spin_unlock(&kmalloc_lock);
    
    obj->magic = 0xDEADBEEF;
    obj->size = slab->size;
    
    return (void*)(obj + 1);
}

void* kzalloc(size_t size) {
    void* ptr = kmalloc(size);
    if (ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

void kfree(void* ptr) {
    if (!ptr) return;
    
    struct slab_header* obj = (struct slab_header*)ptr - 1;
    
    if (obj->magic != 0xDEADBEEF) {
        // Might be a large allocation
        // Try to free as pages
        uint64_t pfn = (uint64_t)ptr / PAGE_SIZE;
        // Just return for now (large allocations need tracking)
        return;
    }
    
    uint32_t size = obj->size;
    
    if (size > KMALLOC_MAX_SIZE) {
        // Large allocation: free pages
        size_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
        pmm_free_pages(ptr, pages);
        return;
    }
    
    int idx = size_to_index(size);
    if (idx < 0) {
        return;
    }
    
    spin_lock(&kmalloc_lock);
    
    struct slab_cache* slab = &slabs[idx];
    obj->next = slab->free_list;
    slab->free_list = obj;
    slab->frees++;
    
    spin_unlock(&kmalloc_lock);
}
