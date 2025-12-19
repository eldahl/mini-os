#include <stdint.h>
#include "types.h"
#include "graphics.h"
#include "font.h"

void kmain(void) {
    struct BootInfo* info = BOOTINFO;
    
    // Safety check
    if (info->fb_addr == 0 || info->fb_width == 0) {
        for (;;) __asm__ volatile ("hlt");
    }
    
    // Initialize graphics
    gfx_init();
    gfx_clear(COLOR_DARK_BG);
    
    // Draw title
    font_draw_string_shadow(20, 20, "MINI-OS GRAPHICS TEST", COLOR_WHITE, COLOR_DARK_GRAY, 3, 2);
    
    // Test shapes
    font_draw_string(20, 80, "Testing basic graphics...", COLOR_GREEN, 1);
    
    gfx_fill_rect(20, 110, 100, 60, COLOR_RED);
    gfx_fill_rect(140, 110, 100, 60, COLOR_GREEN);
    gfx_fill_rect(260, 110, 100, 60, COLOR_BLUE);
    
    gfx_fill_circle(420, 140, 30, COLOR_YELLOW);
    gfx_fill_circle(500, 140, 30, COLOR_CYAN);
    gfx_fill_circle(580, 140, 30, COLOR_MAGENTA);
    
    // VBE info
    font_draw_string(20, 200, "VBE Framebuffer Info:", COLOR_WHITE, 2);
    
    font_draw_string(20, 240, "Address: ", COLOR_GRAY, 1);
    font_draw_hex(110, 240, info->fb_addr, COLOR_NEON_GREEN, 1);
    
    font_draw_string(20, 260, "Width:   ", COLOR_GRAY, 1);
    font_draw_int(110, 260, info->fb_width, COLOR_WHITE, 1);
    
    font_draw_string(20, 280, "Height:  ", COLOR_GRAY, 1);
    font_draw_int(110, 280, info->fb_height, COLOR_WHITE, 1);
    
    font_draw_string(20, 300, "BPP:     ", COLOR_GRAY, 1);
    font_draw_int(110, 300, info->fb_bpp, COLOR_WHITE, 1);
    
    // Success message
    font_draw_string_centered(info->fb_width / 2, info->fb_height - 50,
                              "Graphics Working! Press any key to continue...", COLOR_NEON_GREEN, 2);
    
    // Halt
    for (;;) {
        __asm__ volatile ("hlt");
    }
}
