#include <stdint.h>

// ---------- BootInfo struct ----------

struct BootInfo {
    uint32_t magic;
    uint8_t  boot_drive;
    uint8_t  _pad[3];
    uint32_t kernel_phys;
    uint32_t kernel_sectors;
    uint32_t fb_addr;
    uint16_t fb_pitch;
    uint16_t fb_width;
    uint16_t fb_height;
    uint8_t  fb_bpp;
    uint8_t  fb_type; // 1 = RGB
};

#define BOOTINFO_ADDR 0x00007E00
#define BOOTINFO ((struct BootInfo*)BOOTINFO_ADDR)

void kmain(void) {
    struct BootInfo* info = BOOTINFO;

    // Basic check to avoid writing somewhere random if VBE failed.
    if (info->fb_addr == 0 || info->fb_width == 0 || info->fb_height == 0) {
        for (;;) { __asm__ volatile ("hlt"); }
    }

    // Draw a simple gradient / bars to prove linear framebuffer is mapped.
    for (uint32_t y = 0; y < info->fb_height; ++y) {
        uint8_t* row = (uint8_t*)(uintptr_t)(info->fb_addr + y * info->fb_pitch);
        for (uint32_t x = 0; x < info->fb_width; ++x) {
            // Create a gradient across X/Y and a moving bar.
            uint8_t r = (x * 255) / (info->fb_width ? info->fb_width : 1);
            uint8_t g = (y * 255) / (info->fb_height ? info->fb_height : 1);
            uint8_t b = ((x + y) * 255) / ((info->fb_width + info->fb_height) ? (info->fb_width + info->fb_height) : 1);

            if (info->fb_bpp == 32) {
                uint32_t* p32 = (uint32_t*)row;
                p32[x] = ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
            } else if (info->fb_bpp == 24) {
                uint8_t* p = row + x * 3;
                p[0] = b;
                p[1] = g;
                p[2] = r;
            } else if (info->fb_bpp == 16) {
                uint16_t* p16 = (uint16_t*)row;
                uint16_t rv = ((uint16_t)(r >> 3) << 11) |
                              ((uint16_t)(g >> 2) << 5)  |
                              ((uint16_t)(b >> 3));
                p16[x] = rv;
            }
        }
    }

    for (;;) {
        __asm__ volatile ("hlt");
    }
}
