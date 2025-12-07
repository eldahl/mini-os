#include <stdint.h>

#define VGA_MEMORY ((uint16_t*)0xB8000)
#define VGA_COLS   80
#define VGA_ROWS   25

static uint16_t make_vga_entry(char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}

static void putc_at(int x, int y, char c, uint8_t color) {
    volatile uint16_t* vga = VGA_MEMORY;
    vga[y * VGA_COLS + x] = make_vga_entry(c, color);
}

static void puts_at(int x, int y, const char* s, uint8_t color) {
    int i = 0;
    while (s[i]) {
        putc_at(x + i, y, s[i], color);
        i++;
    }
}

// ---------- BootInfo struct ----------

struct BootInfo {
    uint32_t magic;
    uint8_t  boot_drive;
    uint8_t  _pad[3];
    uint32_t kernel_phys;
    uint32_t kernel_sectors;
};

#define BOOTINFO_ADDR 0x00007E00
#define BOOTINFO ((struct BootInfo*)BOOTINFO_ADDR)

// very dumb hex print (0–F only)
static char hex_digit(uint8_t v) {
    v &= 0xF;
    return v < 10 ? '0' + v : 'A' + (v - 10);
}

static void print_hex32_at(int x, int y, uint32_t val, uint8_t color) {
    for (int i = 0; i < 8; ++i) {
        uint8_t nibble = (val >> ((7 - i) * 4)) & 0xF;
        putc_at(x + i, y, hex_digit(nibble), color);
    }
}

void kmain(void) {
    // Clear screen
    for (int i = 0; i < VGA_COLS * VGA_ROWS; ++i) {
        ((volatile uint16_t*)VGA_MEMORY)[i] = make_vga_entry(' ', 0x0F);
    }

    puts_at(0, 0, "Hello from 32-bit kernel!", 0x0F);

    // Read BootInfo
    struct BootInfo* info = BOOTINFO;

    puts_at(0, 2, "BootInfo.magic:      0x", 0x0F);
    print_hex32_at(22, 2, info->magic, 0x0F);

    puts_at(0, 3, "BootInfo.boot_drive: 0x", 0x0F);
    print_hex32_at(22, 3, info->boot_drive, 0x0F);

    puts_at(0, 4, "BootInfo.kernel_phys:0x", 0x0F);
    print_hex32_at(22, 4, info->kernel_phys, 0x0F);

    puts_at(0, 5, "BootInfo.kernel_secs:0x", 0x0F);
    print_hex32_at(22, 5, info->kernel_sectors, 0x0F);

    for (;;) {
        __asm__ volatile ("hlt");
    }
}

