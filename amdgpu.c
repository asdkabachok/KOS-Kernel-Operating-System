// amdgpu.c — AMD GPU stub
#include "kernel.h"

static void* fb_virt = NULL;
static uint32_t fb_width = 1024;
static uint32_t fb_height = 768;
static uint32_t fb_pitch = 1024 * 4;
static uint32_t fb_bpp = 32;

bool amdgpu_init(void) {
    // Try to find AMD GPU
    struct pci_device* dev = pci_find_device(0x1002, 0x15DD); // Picasso/Vega 8
    if (!dev) dev = pci_find_device(0x1002, 0x15D8); // Raven2
    
    if (!dev) {
        kprintf("amdgpu: AMD GPU not found\n");
        return false;
    }
    
    kprintf("amdgpu: Found AMD GPU at %02x:%02x.%x\n", 
        dev->bus, dev->slot, dev->func);
    kprintf("amdgpu: Driver stub - using VESA mode\n");
    
    // Would initialize framebuffer here in real driver
    return false; // Return false to use VGA text mode
}

void* amdgpu_get_framebuffer(void) {
    return fb_virt;
}

uint32_t amdgpu_get_fb_width(void) { return fb_width; }
uint32_t amdgpu_get_fb_height(void) { return fb_height; }
uint32_t amdgpu_get_fb_pitch(void) { return fb_pitch; }
uint32_t amdgpu_get_fb_bpp(void) { return fb_bpp; }

void amdgpu_putpixel(uint32_t x, uint32_t y, uint32_t color) {
    if (!fb_virt || x >= fb_width || y >= fb_height) return;
    
    uint32_t* fb = (uint32_t*)((uint8_t*)fb_virt + y * fb_pitch);
    fb[x] = color;
}

void amdgpu_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    if (!fb_virt) return;
    
    for (uint32_t py = y; py < y + h && py < fb_height; py++) {
        uint32_t* line = (uint32_t*)((uint8_t*)fb_virt + py * fb_pitch);
        for (uint32_t px = x; px < x + w && px < fb_width; px++) {
            line[px] = color;
        }
    }
}

void amdgpu_clear(uint32_t color) {
    amdgpu_fill_rect(0, 0, fb_width, fb_height, color);
}
