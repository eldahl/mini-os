#ifndef GPU_H
#define GPU_H

#include "types.h"

// ============================================================================
// GPU DRIVER FOR MINI-OS
// Provides hardware abstraction, double buffering, sprites, and rendering
// ============================================================================

// ---------- GPU Device Types ----------
typedef enum {
    GPU_TYPE_UNKNOWN = 0,
    GPU_TYPE_VBE,           // VESA BIOS Extensions (what we're using)
    GPU_TYPE_BOCHS,         // Bochs VBE extensions
    GPU_TYPE_QEMU_STD,      // QEMU standard VGA
} GPUType;

// ---------- Pixel Formats ----------
typedef enum {
    PIXEL_FORMAT_UNKNOWN = 0,
    PIXEL_FORMAT_RGB565,    // 16-bit: 5-6-5
    PIXEL_FORMAT_RGB888,    // 24-bit: 8-8-8
    PIXEL_FORMAT_XRGB8888,  // 32-bit: x-8-8-8
    PIXEL_FORMAT_ARGB8888,  // 32-bit with alpha
} PixelFormat;

// ---------- GPU Device Info ----------
typedef struct {
    GPUType type;
    PixelFormat format;
    uint32_t framebuffer_addr;
    uint32_t framebuffer_size;
    uint16_t width;
    uint16_t height;
    uint16_t pitch;
    uint8_t  bpp;
    uint8_t  bytes_per_pixel;
} GPUDevice;

// ---------- Framebuffer (for double buffering) ----------
typedef struct {
    uint8_t* data;
    uint16_t width;
    uint16_t height;
    uint16_t pitch;
    uint8_t  bpp;
} Framebuffer;

// ---------- Rectangle structure ----------
typedef struct {
    int32_t x, y;
    uint32_t width, height;
} Rect;

// ---------- Sprite structure ----------
typedef struct {
    uint32_t* pixels;       // ARGB pixel data
    uint16_t width;
    uint16_t height;
    int16_t hot_x, hot_y;   // Hotspot for cursors
} Sprite;

// ---------- Viewport/Clipping ----------
typedef struct {
    int32_t x, y;
    uint32_t width, height;
} Viewport;

// ---------- Global GPU state ----------
static GPUDevice g_gpu;
static Framebuffer g_backbuffer;
static Viewport g_viewport;
static uint8_t* g_backbuffer_memory;

// Reserve 3MB for back buffer (enough for 1024x768x32bpp)
#define BACKBUFFER_ADDR 0x00200000
#define BACKBUFFER_SIZE (1024 * 768 * 4)

// ============================================================================
// GPU INITIALIZATION
// ============================================================================

static inline PixelFormat gpu_detect_format(uint8_t bpp) {
    switch (bpp) {
        case 16: return PIXEL_FORMAT_RGB565;
        case 24: return PIXEL_FORMAT_RGB888;
        case 32: return PIXEL_FORMAT_XRGB8888;
        default: return PIXEL_FORMAT_UNKNOWN;
    }
}

static inline int gpu_init(void) {
    struct BootInfo* info = BOOTINFO;
    
    if (info->fb_addr == 0 || info->fb_width == 0 || info->fb_height == 0) {
        return -1; // No framebuffer available
    }
    
    // Initialize GPU device info
    g_gpu.type = GPU_TYPE_VBE;
    g_gpu.format = gpu_detect_format(info->fb_bpp);
    g_gpu.framebuffer_addr = info->fb_addr;
    g_gpu.width = info->fb_width;
    g_gpu.height = info->fb_height;
    g_gpu.pitch = info->fb_pitch;
    g_gpu.bpp = info->fb_bpp;
    g_gpu.bytes_per_pixel = info->fb_bpp / 8;
    g_gpu.framebuffer_size = info->fb_pitch * info->fb_height;
    
    // Initialize back buffer
    g_backbuffer_memory = (uint8_t*)BACKBUFFER_ADDR;
    g_backbuffer.data = g_backbuffer_memory;
    g_backbuffer.width = info->fb_width;
    g_backbuffer.height = info->fb_height;
    g_backbuffer.pitch = info->fb_width * g_gpu.bytes_per_pixel;
    g_backbuffer.bpp = info->fb_bpp;
    
    // Initialize viewport to full screen
    g_viewport.x = 0;
    g_viewport.y = 0;
    g_viewport.width = info->fb_width;
    g_viewport.height = info->fb_height;
    
    // Clear back buffer
    for (uint32_t i = 0; i < g_backbuffer.pitch * g_backbuffer.height; i++) {
        g_backbuffer_memory[i] = 0;
    }
    
    return 0;
}

// ============================================================================
// GPU INFO FUNCTIONS
// ============================================================================

static inline GPUDevice* gpu_get_device(void) {
    return &g_gpu;
}

static inline uint16_t gpu_get_width(void) {
    return g_gpu.width;
}

static inline uint16_t gpu_get_height(void) {
    return g_gpu.height;
}

static inline PixelFormat gpu_get_format(void) {
    return g_gpu.format;
}

static inline const char* gpu_get_type_string(void) {
    switch (g_gpu.type) {
        case GPU_TYPE_VBE: return "VBE";
        case GPU_TYPE_BOCHS: return "Bochs";
        case GPU_TYPE_QEMU_STD: return "QEMU Std";
        default: return "Unknown";
    }
}

static inline const char* gpu_get_format_string(void) {
    switch (g_gpu.format) {
        case PIXEL_FORMAT_RGB565: return "RGB565";
        case PIXEL_FORMAT_RGB888: return "RGB888";
        case PIXEL_FORMAT_XRGB8888: return "XRGB8888";
        case PIXEL_FORMAT_ARGB8888: return "ARGB8888";
        default: return "Unknown";
    }
}

// ============================================================================
// VIEWPORT & CLIPPING
// ============================================================================

static inline void gpu_set_viewport(int32_t x, int32_t y, uint32_t w, uint32_t h) {
    g_viewport.x = x;
    g_viewport.y = y;
    g_viewport.width = w;
    g_viewport.height = h;
}

static inline void gpu_reset_viewport(void) {
    g_viewport.x = 0;
    g_viewport.y = 0;
    g_viewport.width = g_gpu.width;
    g_viewport.height = g_gpu.height;
}

static inline int gpu_clip_point(int32_t* x, int32_t* y) {
    if (*x < g_viewport.x || *x >= (int32_t)(g_viewport.x + g_viewport.width)) return 0;
    if (*y < g_viewport.y || *y >= (int32_t)(g_viewport.y + g_viewport.height)) return 0;
    return 1;
}

static inline int gpu_clip_rect(Rect* r) {
    int32_t x1 = r->x;
    int32_t y1 = r->y;
    int32_t x2 = r->x + r->width;
    int32_t y2 = r->y + r->height;
    
    int32_t vx1 = g_viewport.x;
    int32_t vy1 = g_viewport.y;
    int32_t vx2 = g_viewport.x + g_viewport.width;
    int32_t vy2 = g_viewport.y + g_viewport.height;
    
    if (x1 < vx1) x1 = vx1;
    if (y1 < vy1) y1 = vy1;
    if (x2 > vx2) x2 = vx2;
    if (y2 > vy2) y2 = vy2;
    
    if (x1 >= x2 || y1 >= y2) return 0;
    
    r->x = x1;
    r->y = y1;
    r->width = x2 - x1;
    r->height = y2 - y1;
    return 1;
}

// ============================================================================
// BACK BUFFER PIXEL OPERATIONS
// ============================================================================

// GPU-specific color macros (aliases to common ones)
#define GPU_RGB(r, g, b)     RGB(r, g, b)
#define GPU_RGBA(r, g, b, a) RGBA(r, g, b, a)
#define GPU_GET_R(c)         GET_R(c)
#define GPU_GET_G(c)         GET_G(c)
#define GPU_GET_B(c)         GET_B(c)
#define GPU_GET_A(c)         GET_A(c)

static inline void gpu_put_pixel(int32_t x, int32_t y, Color color) {
    if (x < 0 || x >= g_backbuffer.width || y < 0 || y >= g_backbuffer.height) return;
    
    uint8_t* row = g_backbuffer.data + y * g_backbuffer.pitch;
    
    if (g_backbuffer.bpp == 32) {
        ((uint32_t*)row)[x] = color;
    } else if (g_backbuffer.bpp == 24) {
        uint8_t* p = row + x * 3;
        p[0] = GET_B(color);
        p[1] = GET_G(color);
        p[2] = GET_R(color);
    } else if (g_backbuffer.bpp == 16) {
        uint16_t* p = (uint16_t*)row;
        p[x] = ((GET_R(color) >> 3) << 11) | 
               ((GET_G(color) >> 2) << 5) | 
               (GET_B(color) >> 3);
    }
}

static inline Color gpu_get_pixel(int32_t x, int32_t y) {
    if (x < 0 || x >= g_backbuffer.width || y < 0 || y >= g_backbuffer.height) return 0;
    
    uint8_t* row = g_backbuffer.data + y * g_backbuffer.pitch;
    
    if (g_backbuffer.bpp == 32) {
        return ((uint32_t*)row)[x];
    } else if (g_backbuffer.bpp == 24) {
        uint8_t* p = row + x * 3;
        return RGB(p[2], p[1], p[0]);
    } else if (g_backbuffer.bpp == 16) {
        uint16_t v = ((uint16_t*)row)[x];
        return RGB(((v >> 11) & 0x1F) << 3, ((v >> 5) & 0x3F) << 2, (v & 0x1F) << 3);
    }
    return 0;
}

// Alpha blending
static inline Color gpu_blend(Color fg, Color bg, uint8_t alpha) {
    uint8_t inv = 255 - alpha;
    return RGB(
        (GET_R(fg) * alpha + GET_R(bg) * inv) / 255,
        (GET_G(fg) * alpha + GET_G(bg) * inv) / 255,
        (GET_B(fg) * alpha + GET_B(bg) * inv) / 255
    );
}

static inline void gpu_put_pixel_alpha(int32_t x, int32_t y, Color color) {
    uint8_t alpha = GET_A(color);
    if (alpha == 0) return;
    if (alpha == 255) {
        gpu_put_pixel(x, y, color);
        return;
    }
    Color bg = gpu_get_pixel(x, y);
    gpu_put_pixel(x, y, gpu_blend(color, bg, alpha));
}

// ============================================================================
// BACK BUFFER OPERATIONS
// ============================================================================

static inline void gpu_clear(Color color) {
    for (int32_t y = 0; y < g_backbuffer.height; y++) {
        for (int32_t x = 0; x < g_backbuffer.width; x++) {
            gpu_put_pixel(x, y, color);
        }
    }
}

static inline void gpu_clear_rect(int32_t x, int32_t y, uint32_t w, uint32_t h, Color color) {
    Rect r = {x, y, w, h};
    if (!gpu_clip_rect(&r)) return;
    
    for (uint32_t dy = 0; dy < r.height; dy++) {
        for (uint32_t dx = 0; dx < r.width; dx++) {
            gpu_put_pixel(r.x + dx, r.y + dy, color);
        }
    }
}

// ============================================================================
// PRESENT / FLIP (Copy back buffer to front buffer)
// ============================================================================

static inline void gpu_present(void) {
    uint8_t* src = g_backbuffer.data;
    uint8_t* dst = (uint8_t*)(uintptr_t)g_gpu.framebuffer_addr;
    
    // Copy each row (handles different pitches)
    uint32_t row_bytes = g_backbuffer.width * g_gpu.bytes_per_pixel;
    for (uint32_t y = 0; y < g_backbuffer.height; y++) {
        uint8_t* src_row = src + y * g_backbuffer.pitch;
        uint8_t* dst_row = dst + y * g_gpu.pitch;
        for (uint32_t i = 0; i < row_bytes; i++) {
            dst_row[i] = src_row[i];
        }
    }
}

// Present only a dirty region (optimization)
static inline void gpu_present_rect(int32_t x, int32_t y, uint32_t w, uint32_t h) {
    Rect r = {x, y, w, h};
    if (!gpu_clip_rect(&r)) return;
    
    uint8_t* src = g_backbuffer.data;
    uint8_t* dst = (uint8_t*)(uintptr_t)g_gpu.framebuffer_addr;
    
    for (uint32_t dy = 0; dy < r.height; dy++) {
        uint8_t* src_row = src + (r.y + dy) * g_backbuffer.pitch + r.x * g_gpu.bytes_per_pixel;
        uint8_t* dst_row = dst + (r.y + dy) * g_gpu.pitch + r.x * g_gpu.bytes_per_pixel;
        uint32_t bytes = r.width * g_gpu.bytes_per_pixel;
        for (uint32_t i = 0; i < bytes; i++) {
            dst_row[i] = src_row[i];
        }
    }
}

// ============================================================================
// DRAWING PRIMITIVES (on back buffer)
// ============================================================================

static inline void gpu_draw_hline(int32_t x, int32_t y, uint32_t len, Color color) {
    for (uint32_t i = 0; i < len; i++) {
        gpu_put_pixel(x + i, y, color);
    }
}

static inline void gpu_draw_vline(int32_t x, int32_t y, uint32_t len, Color color) {
    for (uint32_t i = 0; i < len; i++) {
        gpu_put_pixel(x, y + i, color);
    }
}

static inline int32_t gpu_abs(int32_t x) { return x < 0 ? -x : x; }

static inline void gpu_draw_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, Color color) {
    int32_t dx = gpu_abs(x1 - x0);
    int32_t dy = gpu_abs(y1 - y0);
    int32_t sx = x0 < x1 ? 1 : -1;
    int32_t sy = y0 < y1 ? 1 : -1;
    int32_t err = dx - dy;
    
    while (1) {
        gpu_put_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int32_t e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx)  { err += dx; y0 += sy; }
    }
}

static inline void gpu_draw_rect(int32_t x, int32_t y, uint32_t w, uint32_t h, Color color) {
    gpu_draw_hline(x, y, w, color);
    gpu_draw_hline(x, y + h - 1, w, color);
    gpu_draw_vline(x, y, h, color);
    gpu_draw_vline(x + w - 1, y, h, color);
}

static inline void gpu_fill_rect(int32_t x, int32_t y, uint32_t w, uint32_t h, Color color) {
    for (uint32_t dy = 0; dy < h; dy++) {
        for (uint32_t dx = 0; dx < w; dx++) {
            gpu_put_pixel(x + dx, y + dy, color);
        }
    }
}

static inline void gpu_draw_circle(int32_t cx, int32_t cy, int32_t r, Color color) {
    int32_t x = 0, y = r;
    int32_t d = 1 - r;
    
    while (x <= y) {
        gpu_put_pixel(cx + x, cy + y, color);
        gpu_put_pixel(cx - x, cy + y, color);
        gpu_put_pixel(cx + x, cy - y, color);
        gpu_put_pixel(cx - x, cy - y, color);
        gpu_put_pixel(cx + y, cy + x, color);
        gpu_put_pixel(cx - y, cy + x, color);
        gpu_put_pixel(cx + y, cy - x, color);
        gpu_put_pixel(cx - y, cy - x, color);
        
        if (d < 0) { d += 2 * x + 3; }
        else { d += 2 * (x - y) + 5; y--; }
        x++;
    }
}

static inline void gpu_fill_circle(int32_t cx, int32_t cy, int32_t r, Color color) {
    for (int32_t y = -r; y <= r; y++) {
        for (int32_t x = -r; x <= r; x++) {
            if (x * x + y * y <= r * r) {
                gpu_put_pixel(cx + x, cy + y, color);
            }
        }
    }
}

// ============================================================================
// SPRITE / BITMAP RENDERING
// ============================================================================

static inline void gpu_blit_sprite(const Sprite* sprite, int32_t x, int32_t y) {
    if (!sprite || !sprite->pixels) return;
    
    for (int32_t sy = 0; sy < sprite->height; sy++) {
        for (int32_t sx = 0; sx < sprite->width; sx++) {
            Color pixel = sprite->pixels[sy * sprite->width + sx];
            uint8_t alpha = GET_A(pixel);
            if (alpha > 0) {
                if (alpha == 255) {
                    gpu_put_pixel(x + sx, y + sy, pixel);
                } else {
                    gpu_put_pixel_alpha(x + sx, y + sy, pixel);
                }
            }
        }
    }
}

static inline void gpu_blit_sprite_scaled(const Sprite* sprite, int32_t x, int32_t y, 
                                          uint32_t dst_w, uint32_t dst_h) {
    if (!sprite || !sprite->pixels || dst_w == 0 || dst_h == 0) return;
    
    for (uint32_t dy = 0; dy < dst_h; dy++) {
        uint32_t sy = (dy * sprite->height) / dst_h;
        for (uint32_t dx = 0; dx < dst_w; dx++) {
            uint32_t sx = (dx * sprite->width) / dst_w;
            Color pixel = sprite->pixels[sy * sprite->width + sx];
            if (GET_A(pixel) > 0) {
                gpu_put_pixel_alpha(x + dx, y + dy, pixel);
            }
        }
    }
}

static inline void gpu_blit_sprite_region(const Sprite* sprite, int32_t dx, int32_t dy,
                                          int32_t sx, int32_t sy, uint32_t sw, uint32_t sh) {
    if (!sprite || !sprite->pixels) return;
    
    for (uint32_t ry = 0; ry < sh; ry++) {
        for (uint32_t rx = 0; rx < sw; rx++) {
            int32_t src_x = sx + rx;
            int32_t src_y = sy + ry;
            if (src_x >= 0 && src_x < sprite->width && src_y >= 0 && src_y < sprite->height) {
                Color pixel = sprite->pixels[src_y * sprite->width + src_x];
                if (GET_A(pixel) > 0) {
                    gpu_put_pixel_alpha(dx + rx, dy + ry, pixel);
                }
            }
        }
    }
}

// ============================================================================
// BITMAP CREATION HELPERS
// ============================================================================

static inline void gpu_create_solid_sprite(Sprite* sprite, uint16_t w, uint16_t h, 
                                           Color color, uint32_t* buffer) {
    sprite->width = w;
    sprite->height = h;
    sprite->pixels = buffer;
    sprite->hot_x = 0;
    sprite->hot_y = 0;
    
    for (uint32_t i = 0; i < w * h; i++) {
        buffer[i] = color | 0xFF000000;
    }
}

static inline void gpu_create_gradient_sprite(Sprite* sprite, uint16_t w, uint16_t h,
                                              Color c1, Color c2, int vertical, uint32_t* buffer) {
    sprite->width = w;
    sprite->height = h;
    sprite->pixels = buffer;
    sprite->hot_x = 0;
    sprite->hot_y = 0;
    
    for (uint16_t y = 0; y < h; y++) {
        for (uint16_t x = 0; x < w; x++) {
            uint8_t t = vertical ? (y * 255 / h) : (x * 255 / w);
            uint8_t inv = 255 - t;
            Color c = RGB(
                (GET_R(c1) * inv + GET_R(c2) * t) / 255,
                (GET_G(c1) * inv + GET_G(c2) * t) / 255,
                (GET_B(c1) * inv + GET_B(c2) * t) / 255
            );
            buffer[y * w + x] = c | 0xFF000000;
        }
    }
}

// ============================================================================
// VSYNC (best effort - no real hardware sync without PCI access)
// ============================================================================

static inline void gpu_wait_vsync(void) {
    for (volatile uint32_t i = 0; i < 100000; i++);
}

// ============================================================================
// MEMORY OPERATIONS (fast fills)
// ============================================================================

static inline void gpu_memset32(uint32_t* dst, uint32_t val, uint32_t count) {
    while (count--) *dst++ = val;
}

static inline void gpu_memcpy32(uint32_t* dst, const uint32_t* src, uint32_t count) {
    while (count--) *dst++ = *src++;
}

static inline void gpu_fast_clear(Color color) {
    if (g_backbuffer.bpp == 32) {
        gpu_memset32((uint32_t*)g_backbuffer.data, color, 
                     g_backbuffer.width * g_backbuffer.height);
    } else {
        gpu_clear(color);
    }
}

#endif // GPU_H
