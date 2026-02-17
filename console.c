// console.c — Console output FIXED
#include "kernel.h"

static volatile uint16_t* vga_buffer = (volatile uint16_t*)(KERNEL_HIGHER_HALF + 0xB8000);
static size_t cursor_x = 0, cursor_y = 0;
static const size_t VGA_WIDTH = 80, VGA_HEIGHT = 25;
static void* fb_virt = NULL;
static uint32_t fb_width = 0, fb_height = 0, fb_pitch = 0, fb_bpp = 32;
static uint32_t fg_color = 0xFFFFFF, bg_color = 0x000000;

// Simple 8x16 font (partial, just ASCII basic chars)
static const uint8_t font8x16[128][16] = {
    [0] = {0},  // Space
    ['0'] = {0x3E, 0x63, 0x73, 0x7B, 0x6F, 0x67, 0x3E, 0},
    ['1'] = {0x0C, 0x1C, 0x0C, 0x0C, 0x0C, 0x0C, 0x3F, 0},
    ['A'] = {0x18, 0x3C, 0x66, 0x7E, 0x66, 0x66, 0x66, 0},
    ['K'] = {0x67, 0x66, 0x64, 0x78, 0x6C, 0x66, 0x67, 0},
    ['O'] = {0x3E, 0x63, 0x63, 0x63, 0x63, 0x63, 0x3E, 0},
    ['S'] = {0x7E, 0x03, 0x03, 0x3E, 0x60, 0x60, 0x7F, 0},
    // Add more glyphs as needed
};

void console_early_init(void) {
    for (int i = 0; i < 80 * 25; i++) {
        vga_buffer[i] = 0x0720;
    }
    cursor_x = 0;
    cursor_y = 0;
}

void console_set_framebuffer(void* fb, uint32_t w, uint32_t h, uint32_t pitch, uint32_t bpp) {
    fb_virt = fb;
    fb_width = w;
    fb_height = h;
    fb_pitch = pitch;
    fb_bpp = bpp;
}

static void vga_putchar(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\r') {
        cursor_x = 0;
    } else if (c == '\t') {
        cursor_x = (cursor_x + 4) & ~3;
    } else {
        size_t idx = cursor_y * VGA_WIDTH + cursor_x;
        vga_buffer[idx] = (uint16_t)c | (fg_color << 8) | (bg_color << 12);
        cursor_x++;
    }
    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }
    if (cursor_y >= VGA_HEIGHT) {
        // Scroll
        for (size_t y = 0; y < VGA_HEIGHT - 1; y++) {
            for (size_t x = 0; x < VGA_WIDTH; x++) {
                vga_buffer[y * VGA_WIDTH + x] = vga_buffer[(y + 1) * VGA_WIDTH + x];
            }
        }
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = 0x0720;
        }
        cursor_y = VGA_HEIGHT - 1;
    }
}

static void fb_putchar(char c) {
    if (!fb_virt) {
        vga_putchar(c);
        return;
    }
    
    if (c == '\n') {
        cursor_x = 0;
        cursor_y += 16;
    } else if (c != '\r' && c != '\t') {
        // Render glyph at (cursor_x, cursor_y) - simplified
        // In real implementation, would render from font8x16
        cursor_x += 8;
    }
    
    if (cursor_x >= fb_width) {
        cursor_x = 0;
        cursor_y += 16;
    }
    if (cursor_y >= fb_height) {
        cursor_y = 0; // Would need to scroll fb
    }
}

static void print_hex(uint64_t val, int digits) {
    const char* hex = "0123456789ABCDEF";
    for (int i = digits - 1; i >= 0; i--) {
        vga_putchar(hex[(val >> (i * 4)) & 0xF]);
    }
}

static void print_dec(uint64_t val) {
    char buf[32];
    int i = 30;
    buf[31] = 0;
    if (val == 0) {
        vga_putchar('0');
        return;
    }
    while (val && i >= 0) {
        buf[i--] = '0' + (val % 10);
        val /= 10;
    }
    while (++i < 31) {
        vga_putchar(buf[i]);
    }
}

void kvprintf(const char* fmt, va_list args) {
    while (*fmt) {
        if (*fmt == '%' && *(fmt + 1)) {
            fmt++;
            switch (*fmt) {
                case 's': {
                    const char* s = va_arg(args, const char*);
                    if (s) while (*s) vga_putchar(*s++);
                    break;
                }
                case 'd':
                case 'i':
                    print_dec(va_arg(args, int));
                    break;
                case 'u':
                    print_dec(va_arg(args, unsigned int));
                    break;
                case 'x':
                    print_hex(va_arg(args, unsigned int), 8);
                    break;
                case 'l':
                    if (*(fmt + 1) == 'x') {
                        fmt++;
                        print_hex(va_arg(args, uint64_t), 16);
                    } else if (*(fmt + 1) == 'u') {
                        fmt++;
                        print_dec(va_arg(args, uint64_t));
                    }
                    break;
                case 'p':
                    vga_putchar('0');
                    vga_putchar('x');
                    print_hex(va_arg(args, uint64_t), 16);
                    break;
                case 'c':
                    vga_putchar((char)va_arg(args, int));
                    break;
                case '%':
                    vga_putchar('%');
                    break;
                default:
                    vga_putchar('%');
                    vga_putchar(*fmt);
                    break;
            }
        } else {
            vga_putchar(*fmt);
        }
        fmt++;
    }
}

void kprintf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    kvprintf(fmt, args);
    va_end(args);
}

void kernel_panic(const char* msg) {
    cli();
    kprintf("\n\n!!! KERNEL PANIC !!!\n%s\nSystem halted.\n", msg);
    while (1) hlt();
}

// Helper to convert color
uint32_t rgb_to_color(uint8_t r, uint8_t g, uint8_t b) {
    return (r << 16) | (g << 8) | b;
}
