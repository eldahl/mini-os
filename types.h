#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

// ============================================================================
// COMMON TYPES FOR MINI-OS
// ============================================================================

// ---------- BootInfo struct (passed from bootloader) ----------
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
    uint8_t  fb_type;
};

#define BOOTINFO_ADDR 0x00007E00
#define BOOTINFO ((struct BootInfo*)BOOTINFO_ADDR)

// ---------- Color type ----------
typedef uint32_t Color;

#define RGB(r, g, b)    (((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b))
#define RGBA(r, g, b, a) (((uint32_t)(a) << 24) | ((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b))

#define GET_R(c) (((c) >> 16) & 0xFF)
#define GET_G(c) (((c) >> 8) & 0xFF)
#define GET_B(c) ((c) & 0xFF)
#define GET_A(c) (((c) >> 24) & 0xFF)

// ---------- Common colors ----------
#define COLOR_BLACK       RGB(0, 0, 0)
#define COLOR_WHITE       RGB(255, 255, 255)
#define COLOR_RED         RGB(255, 0, 0)
#define COLOR_GREEN       RGB(0, 255, 0)
#define COLOR_BLUE        RGB(0, 0, 255)
#define COLOR_YELLOW      RGB(255, 255, 0)
#define COLOR_CYAN        RGB(0, 255, 255)
#define COLOR_MAGENTA     RGB(255, 0, 255)
#define COLOR_ORANGE      RGB(255, 165, 0)
#define COLOR_PURPLE      RGB(128, 0, 128)
#define COLOR_PINK        RGB(255, 192, 203)
#define COLOR_GRAY        RGB(128, 128, 128)
#define COLOR_DARK_GRAY   RGB(64, 64, 64)
#define COLOR_LIGHT_GRAY  RGB(192, 192, 192)

// Neon palette
#define COLOR_NEON_PINK   RGB(255, 16, 240)
#define COLOR_NEON_BLUE   RGB(0, 255, 255)
#define COLOR_NEON_GREEN  RGB(57, 255, 20)
#define COLOR_NEON_PURPLE RGB(191, 0, 255)
#define COLOR_DARK_BG     RGB(10, 10, 25)

#endif // TYPES_H

