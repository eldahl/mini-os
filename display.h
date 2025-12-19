#ifndef DISPLAY_H
#define DISPLAY_H

#include "gpu.h"

// ============================================================================
// DISPLAY MANAGER FOR MINI-OS
// High-level display management, layers, and rendering pipeline
// ============================================================================

// ---------- Display Layer Types ----------
typedef enum {
    LAYER_BACKGROUND = 0,
    LAYER_MAIN,
    LAYER_UI,
    LAYER_OVERLAY,
    LAYER_CURSOR,
    LAYER_COUNT
} LayerType;

// ---------- Layer Structure ----------
typedef struct {
    int32_t x, y;           // Position
    uint32_t width, height; // Size
    uint8_t visible;        // Visibility flag
    uint8_t alpha;          // Layer alpha (0-255)
    uint32_t* buffer;       // Pixel buffer (ARGB)
    uint8_t dirty;          // Needs redraw
} Layer;

// ---------- Display State ----------
typedef struct {
    uint16_t width;
    uint16_t height;
    uint32_t frame_count;
    uint32_t fps;
    uint32_t last_fps_time;
    uint32_t frame_time;
    Layer layers[LAYER_COUNT];
    uint8_t cursor_visible;
    int32_t cursor_x, cursor_y;
} Display;

static Display g_display;

// Layer buffers - statically allocated at known addresses
#define LAYER_BG_ADDR     0x00300000
#define LAYER_MAIN_ADDR   0x00400000
#define LAYER_UI_ADDR     0x00500000
#define LAYER_OVERLAY_ADDR 0x00600000
#define LAYER_CURSOR_ADDR 0x00700000

// ============================================================================
// DISPLAY INITIALIZATION
// ============================================================================

static inline int display_init(void) {
    if (gpu_init() != 0) {
        return -1;
    }
    
    g_display.width = gpu_get_width();
    g_display.height = gpu_get_height();
    g_display.frame_count = 0;
    g_display.fps = 0;
    g_display.last_fps_time = 0;
    g_display.frame_time = 0;
    g_display.cursor_visible = 0;
    g_display.cursor_x = g_display.width / 2;
    g_display.cursor_y = g_display.height / 2;
    
    // Initialize layers
    uint32_t* layer_addrs[] = {
        (uint32_t*)LAYER_BG_ADDR,
        (uint32_t*)LAYER_MAIN_ADDR,
        (uint32_t*)LAYER_UI_ADDR,
        (uint32_t*)LAYER_OVERLAY_ADDR,
        (uint32_t*)LAYER_CURSOR_ADDR
    };
    
    for (int i = 0; i < LAYER_COUNT; i++) {
        g_display.layers[i].x = 0;
        g_display.layers[i].y = 0;
        g_display.layers[i].width = g_display.width;
        g_display.layers[i].height = g_display.height;
        g_display.layers[i].visible = (i == LAYER_BACKGROUND || i == LAYER_MAIN);
        g_display.layers[i].alpha = 255;
        g_display.layers[i].buffer = layer_addrs[i];
        g_display.layers[i].dirty = 1;
        
        // Clear layer buffer
        uint32_t size = g_display.width * g_display.height;
        for (uint32_t j = 0; j < size; j++) {
            layer_addrs[i][j] = 0;
        }
    }
    
    // Cursor layer is smaller
    g_display.layers[LAYER_CURSOR].width = 16;
    g_display.layers[LAYER_CURSOR].height = 16;
    
    return 0;
}

// ============================================================================
// LAYER OPERATIONS
// ============================================================================

static inline Layer* display_get_layer(LayerType type) {
    if (type >= LAYER_COUNT) return 0;
    return &g_display.layers[type];
}

static inline void display_layer_set_visible(LayerType type, uint8_t visible) {
    if (type >= LAYER_COUNT) return;
    g_display.layers[type].visible = visible;
    g_display.layers[type].dirty = 1;
}

static inline void display_layer_set_alpha(LayerType type, uint8_t alpha) {
    if (type >= LAYER_COUNT) return;
    g_display.layers[type].alpha = alpha;
    g_display.layers[type].dirty = 1;
}

static inline void display_layer_set_position(LayerType type, int32_t x, int32_t y) {
    if (type >= LAYER_COUNT) return;
    g_display.layers[type].x = x;
    g_display.layers[type].y = y;
    g_display.layers[type].dirty = 1;
}

static inline void display_layer_clear(LayerType type, Color color) {
    if (type >= LAYER_COUNT) return;
    Layer* layer = &g_display.layers[type];
    uint32_t size = layer->width * layer->height;
    for (uint32_t i = 0; i < size; i++) {
        layer->buffer[i] = color;
    }
    layer->dirty = 1;
}

static inline void display_layer_put_pixel(LayerType type, int32_t x, int32_t y, Color color) {
    if (type >= LAYER_COUNT) return;
    Layer* layer = &g_display.layers[type];
    if (x < 0 || x >= (int32_t)layer->width || y < 0 || y >= (int32_t)layer->height) return;
    layer->buffer[y * layer->width + x] = color;
}

static inline void display_layer_fill_rect(LayerType type, int32_t x, int32_t y, 
                                           uint32_t w, uint32_t h, Color color) {
    if (type >= LAYER_COUNT) return;
    Layer* layer = &g_display.layers[type];
    
    for (uint32_t dy = 0; dy < h; dy++) {
        for (uint32_t dx = 0; dx < w; dx++) {
            int32_t px = x + dx;
            int32_t py = y + dy;
            if (px >= 0 && px < (int32_t)layer->width && py >= 0 && py < (int32_t)layer->height) {
                layer->buffer[py * layer->width + px] = color;
            }
        }
    }
    layer->dirty = 1;
}

// ============================================================================
// COMPOSITING & RENDERING
// ============================================================================

// Composite all visible layers to the GPU back buffer
static inline void display_composite(void) {
    // Start with background
    for (int32_t y = 0; y < g_display.height; y++) {
        for (int32_t x = 0; x < g_display.width; x++) {
            Color pixel = 0;
            
            // Blend layers from bottom to top
            for (int i = 0; i < LAYER_COUNT; i++) {
                Layer* layer = &g_display.layers[i];
                if (!layer->visible) continue;
                
                int32_t lx = x - layer->x;
                int32_t ly = y - layer->y;
                
                if (lx >= 0 && lx < (int32_t)layer->width && 
                    ly >= 0 && ly < (int32_t)layer->height) {
                    Color src = layer->buffer[ly * layer->width + lx];
                    uint8_t src_alpha = GET_A(src);
                    
                    if (layer->alpha < 255) {
                        src_alpha = (src_alpha * layer->alpha) / 255;
                    }
                    
                    if (src_alpha == 255) {
                        pixel = src;
                    } else if (src_alpha > 0) {
                        pixel = gpu_blend(src, pixel, src_alpha);
                    }
                }
            }
            
            gpu_put_pixel(x, y, pixel);
        }
    }
}

// ============================================================================
// FRAME MANAGEMENT
// ============================================================================

// Simple tick counter (increments each frame)
static volatile uint32_t g_tick_count = 0;

static inline uint32_t display_get_ticks(void) {
    return g_tick_count;
}

static inline void display_begin_frame(void) {
    g_display.frame_count++;
    g_tick_count++;
}

static inline void display_end_frame(void) {
    // Composite layers
    display_composite();
    
    // Present to screen
    gpu_present();
    
    // Wait for vsync (approximate)
    gpu_wait_vsync();
}

// Quick present without compositing (direct GPU buffer)
static inline void display_present_direct(void) {
    gpu_present();
}

// ============================================================================
// CURSOR SUPPORT
// ============================================================================

static inline void display_set_cursor_visible(uint8_t visible) {
    g_display.cursor_visible = visible;
    display_layer_set_visible(LAYER_CURSOR, visible);
}

static inline void display_set_cursor_position(int32_t x, int32_t y) {
    g_display.cursor_x = x;
    g_display.cursor_y = y;
    display_layer_set_position(LAYER_CURSOR, x, y);
}

static inline void display_create_default_cursor(void) {
    Layer* cursor = &g_display.layers[LAYER_CURSOR];
    
    // Simple arrow cursor (16x16)
    static const uint8_t cursor_data[16][16] = {
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {1,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {1,2,2,1,0,0,0,0,0,0,0,0,0,0,0,0},
        {1,2,2,2,1,0,0,0,0,0,0,0,0,0,0,0},
        {1,2,2,2,2,1,0,0,0,0,0,0,0,0,0,0},
        {1,2,2,2,2,2,1,0,0,0,0,0,0,0,0,0},
        {1,2,2,2,2,2,2,1,0,0,0,0,0,0,0,0},
        {1,2,2,2,2,2,2,2,1,0,0,0,0,0,0,0},
        {1,2,2,2,2,2,1,1,1,1,0,0,0,0,0,0},
        {1,2,2,1,2,2,1,0,0,0,0,0,0,0,0,0},
        {1,2,1,0,1,2,2,1,0,0,0,0,0,0,0,0},
        {1,1,0,0,1,2,2,1,0,0,0,0,0,0,0,0},
        {1,0,0,0,0,1,2,2,1,0,0,0,0,0,0,0},
        {0,0,0,0,0,1,2,2,1,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0},
    };
    
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            Color c = 0; // Transparent
            if (cursor_data[y][x] == 1) c = 0xFF000000; // Black outline
            else if (cursor_data[y][x] == 2) c = 0xFFFFFFFF; // White fill
            cursor->buffer[y * cursor->width + x] = c;
        }
    }
}

// ============================================================================
// DISPLAY INFO
// ============================================================================

static inline uint16_t display_get_width(void) { return g_display.width; }
static inline uint16_t display_get_height(void) { return g_display.height; }
static inline uint32_t display_get_frame_count(void) { return g_display.frame_count; }

#endif // DISPLAY_H
