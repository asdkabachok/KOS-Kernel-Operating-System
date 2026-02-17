// ext2.c — Ext2 filesystem stub
#include "kernel.h"

static bool ext2_mounted = false;
static uint64_t ext2_start_lba = 0;

bool ext2_mount(bool (*read_fn)(uint64_t, uint32_t, void*), 
                bool (*write_fn)(uint64_t, uint32_t, const void*), 
                uint64_t start_lba) {
    (void)read_fn;
    (void)write_fn;
    
    kprintf("Ext2: Mount attempt at LBA %lu (stub)\n", start_lba);
    
    // Would actually parse superblock and verify ext2 magic
    ext2_mounted = false;
    ext2_start_lba = start_lba;
    
    return false; // Stub always fails
}

bool ext2_open(const char* path, void* file) {
    (void)path;
    (void)file;
    
    if (!ext2_mounted) return false;
    
    // Would actually open file
    return false;
}

uint32_t ext2_read(void* file, void* buf, uint32_t size) {
    (void)file;
    (void)buf;
    
    if (!ext2_mounted) return 0;
    
    return 0;
}

bool ext2_exists(const char* path) {
    (void)path;
    return false;
}
