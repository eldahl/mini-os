#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "types.h"

// ============================================================================
// GRAPHICS LIBRARY FOR MINI-OS
// Provides drawing primitives, shapes, text rendering, and utilities
// ============================================================================

// ---------- Graphics context ----------
typedef struct {
    uint8_t* framebuffer;
    uint16_t width;
    uint16_t height;
    uint16_t pitch;
    uint8_t  bpp;
} GraphicsContext;

static GraphicsContext g_ctx;

// ---------- Initialization ----------
static inline void gfx_init(void) {
    struct BootInfo* info = BOOTINFO;
    g_ctx.framebuffer = (uint8_t*)(uintptr_t)info->fb_addr;
    g_ctx.width = info->fb_width;
    g_ctx.height = info->fb_height;
    g_ctx.pitch = info->fb_pitch;
    g_ctx.bpp = info->fb_bpp;
}

// ---------- Basic pixel operations ----------
static inline void gfx_put_pixel(int x, int y, Color color) {
    if (x < 0 || x >= g_ctx.width || y < 0 || y >= g_ctx.height) return;
    
    uint8_t* row = g_ctx.framebuffer + y * g_ctx.pitch;
    
    if (g_ctx.bpp == 32) {
        uint32_t* p = (uint32_t*)row;
        p[x] = color;
    } else if (g_ctx.bpp == 24) {
        uint8_t* p = row + x * 3;
        p[0] = GET_B(color);
        p[1] = GET_G(color);
        p[2] = GET_R(color);
    } else if (g_ctx.bpp == 16) {
        uint16_t* p = (uint16_t*)row;
        uint16_t r5 = (GET_R(color) >> 3) & 0x1F;
        uint16_t g6 = (GET_G(color) >> 2) & 0x3F;
        uint16_t b5 = (GET_B(color) >> 3) & 0x1F;
        p[x] = (r5 << 11) | (g6 << 5) | b5;
    }
}

static inline Color gfx_get_pixel(int x, int y) {
    if (x < 0 || x >= g_ctx.width || y < 0 || y >= g_ctx.height) return 0;
    
    uint8_t* row = g_ctx.framebuffer + y * g_ctx.pitch;
    
    if (g_ctx.bpp == 32) {
        return ((uint32_t*)row)[x];
    } else if (g_ctx.bpp == 24) {
        uint8_t* p = row + x * 3;
        return RGB(p[2], p[1], p[0]);
    } else if (g_ctx.bpp == 16) {
        uint16_t v = ((uint16_t*)row)[x];
        uint8_t r = ((v >> 11) & 0x1F) << 3;
        uint8_t g = ((v >> 5) & 0x3F) << 2;
        uint8_t b = (v & 0x1F) << 3;
        return RGB(r, g, b);
    }
    return 0;
}

// ---------- Color utilities ----------
static inline Color color_blend(Color fg, Color bg, uint8_t alpha) {
    uint8_t inv_alpha = 255 - alpha;
    uint8_t r = (GET_R(fg) * alpha + GET_R(bg) * inv_alpha) / 255;
    uint8_t g = (GET_G(fg) * alpha + GET_G(bg) * inv_alpha) / 255;
    uint8_t b = (GET_B(fg) * alpha + GET_B(bg) * inv_alpha) / 255;
    return RGB(r, g, b);
}

static inline Color color_lerp(Color c1, Color c2, uint8_t t) {
    uint8_t inv_t = 255 - t;
    uint8_t r = (GET_R(c1) * inv_t + GET_R(c2) * t) / 255;
    uint8_t g = (GET_G(c1) * inv_t + GET_G(c2) * t) / 255;
    uint8_t b = (GET_B(c1) * inv_t + GET_B(c2) * t) / 255;
    return RGB(r, g, b);
}

static inline Color color_darken(Color c, uint8_t amount) {
    int r = GET_R(c) - amount; if (r < 0) r = 0;
    int g = GET_G(c) - amount; if (g < 0) g = 0;
    int b = GET_B(c) - amount; if (b < 0) b = 0;
    return RGB(r, g, b);
}

static inline Color color_lighten(Color c, uint8_t amount) {
    int r = GET_R(c) + amount; if (r > 255) r = 255;
    int g = GET_G(c) + amount; if (g > 255) g = 255;
    int b = GET_B(c) + amount; if (b > 255) b = 255;
    return RGB(r, g, b);
}

// HSV to RGB conversion (h: 0-360, s: 0-255, v: 0-255)
static inline Color color_from_hsv(int h, uint8_t s, uint8_t v) {
    if (s == 0) return RGB(v, v, v);
    
    h = h % 360;
    int region = h / 60;
    int remainder = (h - (region * 60)) * 255 / 60;
    
    uint8_t p = (v * (255 - s)) / 255;
    uint8_t q = (v * (255 - (s * remainder) / 255)) / 255;
    uint8_t t = (v * (255 - (s * (255 - remainder)) / 255)) / 255;
    
    switch (region) {
        case 0:  return RGB(v, t, p);
        case 1:  return RGB(q, v, p);
        case 2:  return RGB(p, v, t);
        case 3:  return RGB(p, q, v);
        case 4:  return RGB(t, p, v);
        default: return RGB(v, p, q);
    }
}

// ---------- Screen operations ----------
static inline void gfx_clear(Color color) {
    for (int y = 0; y < g_ctx.height; y++) {
        for (int x = 0; x < g_ctx.width; x++) {
            gfx_put_pixel(x, y, color);
        }
    }
}

static inline void gfx_fill_gradient_v(Color top, Color bottom) {
    for (int y = 0; y < g_ctx.height; y++) {
        uint8_t t = (y * 255) / g_ctx.height;
        Color c = color_lerp(top, bottom, t);
        for (int x = 0; x < g_ctx.width; x++) {
            gfx_put_pixel(x, y, c);
        }
    }
}

static inline void gfx_fill_gradient_h(Color left, Color right) {
    for (int y = 0; y < g_ctx.height; y++) {
        for (int x = 0; x < g_ctx.width; x++) {
            uint8_t t = (x * 255) / g_ctx.width;
            gfx_put_pixel(x, y, color_lerp(left, right, t));
        }
    }
}

// ---------- Line drawing (Bresenham's algorithm) ----------
static inline int abs_int(int x) { return x < 0 ? -x : x; }

static inline void gfx_draw_line(int x0, int y0, int x1, int y1, Color color) {
    int dx = abs_int(x1 - x0);
    int dy = abs_int(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    
    while (1) {
        gfx_put_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx)  { err += dx; y0 += sy; }
    }
}

static inline void gfx_draw_line_thick(int x0, int y0, int x1, int y1, int thickness, Color color) {
    int dx = abs_int(x1 - x0);
    int dy = abs_int(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    int half = thickness / 2;
    
    while (1) {
        for (int ty = -half; ty <= half; ty++) {
            for (int tx = -half; tx <= half; tx++) {
                gfx_put_pixel(x0 + tx, y0 + ty, color);
            }
        }
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx)  { err += dx; y0 += sy; }
    }
}

// ---------- Rectangle drawing ----------
static inline void gfx_draw_rect(int x, int y, int w, int h, Color color) {
    gfx_draw_line(x, y, x + w - 1, y, color);
    gfx_draw_line(x, y + h - 1, x + w - 1, y + h - 1, color);
    gfx_draw_line(x, y, x, y + h - 1, color);
    gfx_draw_line(x + w - 1, y, x + w - 1, y + h - 1, color);
}

static inline void gfx_fill_rect(int x, int y, int w, int h, Color color) {
    for (int dy = 0; dy < h; dy++) {
        for (int dx = 0; dx < w; dx++) {
            gfx_put_pixel(x + dx, y + dy, color);
        }
    }
}

static inline void gfx_draw_rect_rounded(int x, int y, int w, int h, int r, Color color) {
    gfx_draw_line(x + r, y, x + w - r - 1, y, color);
    gfx_draw_line(x + r, y + h - 1, x + w - r - 1, y + h - 1, color);
    gfx_draw_line(x, y + r, x, y + h - r - 1, color);
    gfx_draw_line(x + w - 1, y + r, x + w - 1, y + h - r - 1, color);
    
    int cx, cy, d;
    cx = x + r; cy = y + r;
    int px = 0, py = r;
    d = 1 - r;
    while (px <= py) {
        gfx_put_pixel(cx - py, cy - px, color);
        gfx_put_pixel(cx - px, cy - py, color);
        if (d < 0) { d += 2 * px + 3; }
        else { d += 2 * (px - py) + 5; py--; }
        px++;
    }
    cx = x + w - r - 1;
    px = 0; py = r; d = 1 - r;
    while (px <= py) {
        gfx_put_pixel(cx + py, cy - px, color);
        gfx_put_pixel(cx + px, cy - py, color);
        if (d < 0) { d += 2 * px + 3; }
        else { d += 2 * (px - py) + 5; py--; }
        px++;
    }
    cx = x + r; cy = y + h - r - 1;
    px = 0; py = r; d = 1 - r;
    while (px <= py) {
        gfx_put_pixel(cx - py, cy + px, color);
        gfx_put_pixel(cx - px, cy + py, color);
        if (d < 0) { d += 2 * px + 3; }
        else { d += 2 * (px - py) + 5; py--; }
        px++;
    }
    cx = x + w - r - 1;
    px = 0; py = r; d = 1 - r;
    while (px <= py) {
        gfx_put_pixel(cx + py, cy + px, color);
        gfx_put_pixel(cx + px, cy + py, color);
        if (d < 0) { d += 2 * px + 3; }
        else { d += 2 * (px - py) + 5; py--; }
        px++;
    }
}

// ---------- Circle drawing (Midpoint circle algorithm) ----------
static inline void gfx_draw_circle(int cx, int cy, int r, Color color) {
    int x = 0, y = r;
    int d = 1 - r;
    
    while (x <= y) {
        gfx_put_pixel(cx + x, cy + y, color);
        gfx_put_pixel(cx - x, cy + y, color);
        gfx_put_pixel(cx + x, cy - y, color);
        gfx_put_pixel(cx - x, cy - y, color);
        gfx_put_pixel(cx + y, cy + x, color);
        gfx_put_pixel(cx - y, cy + x, color);
        gfx_put_pixel(cx + y, cy - x, color);
        gfx_put_pixel(cx - y, cy - x, color);
        
        if (d < 0) {
            d += 2 * x + 3;
        } else {
            d += 2 * (x - y) + 5;
            y--;
        }
        x++;
    }
}

static inline void gfx_fill_circle(int cx, int cy, int r, Color color) {
    int x = 0, y = r;
    int d = 1 - r;
    
    while (x <= y) {
        for (int i = cx - x; i <= cx + x; i++) {
            gfx_put_pixel(i, cy + y, color);
            gfx_put_pixel(i, cy - y, color);
        }
        for (int i = cx - y; i <= cx + y; i++) {
            gfx_put_pixel(i, cy + x, color);
            gfx_put_pixel(i, cy - x, color);
        }
        
        if (d < 0) {
            d += 2 * x + 3;
        } else {
            d += 2 * (x - y) + 5;
            y--;
        }
        x++;
    }
}

static inline void gfx_draw_ring(int cx, int cy, int r_outer, int r_inner, Color color) {
    for (int y = -r_outer; y <= r_outer; y++) {
        for (int x = -r_outer; x <= r_outer; x++) {
            int dist_sq = x * x + y * y;
            if (dist_sq <= r_outer * r_outer && dist_sq >= r_inner * r_inner) {
                gfx_put_pixel(cx + x, cy + y, color);
            }
        }
    }
}

// ---------- Ellipse drawing ----------
static inline void gfx_draw_ellipse(int cx, int cy, int rx, int ry, Color color) {
    int rx2 = rx * rx;
    int ry2 = ry * ry;
    int tworx2 = 2 * rx2;
    int twory2 = 2 * ry2;
    int x = 0, y = ry;
    int px = 0, py = tworx2 * y;
    
    int p = (int)(ry2 - rx2 * ry + 0.25 * rx2);
    while (px < py) {
        gfx_put_pixel(cx + x, cy + y, color);
        gfx_put_pixel(cx - x, cy + y, color);
        gfx_put_pixel(cx + x, cy - y, color);
        gfx_put_pixel(cx - x, cy - y, color);
        x++;
        px += twory2;
        if (p < 0) {
            p += ry2 + px;
        } else {
            y--;
            py -= tworx2;
            p += ry2 + px - py;
        }
    }
    
    p = (int)(ry2 * (x + 0.5) * (x + 0.5) + rx2 * (y - 1) * (y - 1) - rx2 * ry2);
    while (y >= 0) {
        gfx_put_pixel(cx + x, cy + y, color);
        gfx_put_pixel(cx - x, cy + y, color);
        gfx_put_pixel(cx + x, cy - y, color);
        gfx_put_pixel(cx - x, cy - y, color);
        y--;
        py -= tworx2;
        if (p > 0) {
            p += rx2 - py;
        } else {
            x++;
            px += twory2;
            p += rx2 - py + px;
        }
    }
}

static inline void gfx_fill_ellipse(int cx, int cy, int rx, int ry, Color color) {
    for (int y = -ry; y <= ry; y++) {
        for (int x = -rx; x <= rx; x++) {
            if ((x * x * ry * ry + y * y * rx * rx) <= rx * rx * ry * ry) {
                gfx_put_pixel(cx + x, cy + y, color);
            }
        }
    }
}

// ---------- Triangle drawing ----------
static inline void gfx_draw_triangle(int x0, int y0, int x1, int y1, int x2, int y2, Color color) {
    gfx_draw_line(x0, y0, x1, y1, color);
    gfx_draw_line(x1, y1, x2, y2, color);
    gfx_draw_line(x2, y2, x0, y0, color);
}

static inline void swap_int(int* a, int* b) { int t = *a; *a = *b; *b = t; }

static inline void gfx_fill_triangle(int x0, int y0, int x1, int y1, int x2, int y2, Color color) {
    if (y0 > y1) { swap_int(&x0, &x1); swap_int(&y0, &y1); }
    if (y1 > y2) { swap_int(&x1, &x2); swap_int(&y1, &y2); }
    if (y0 > y1) { swap_int(&x0, &x1); swap_int(&y0, &y1); }
    
    if (y0 == y2) return;
    
    for (int y = y0; y <= y2; y++) {
        int xa, xb;
        
        if (y < y1) {
            if (y1 == y0) continue;
            xa = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
            xb = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
        } else {
            if (y2 == y1) {
                xa = x1;
            } else {
                xa = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
            }
            xb = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
        }
        
        if (xa > xb) swap_int(&xa, &xb);
        for (int x = xa; x <= xb; x++) {
            gfx_put_pixel(x, y, color);
        }
    }
}

// ---------- Polygon drawing ----------
static inline void gfx_draw_polygon(int* points, int num_points, Color color) {
    for (int i = 0; i < num_points; i++) {
        int next = (i + 1) % num_points;
        gfx_draw_line(points[i*2], points[i*2+1], points[next*2], points[next*2+1], color);
    }
}

// ---------- Arc and trigonometry ----------
static inline int sin_approx(int angle) {
    angle = angle % 360;
    if (angle < 0) angle += 360;
    
    int quadrant = angle / 90;
    int a = angle % 90;
    int x = a * 256 / 90;
    int y = (x * (512 - x)) / 256;
    
    switch (quadrant) {
        case 0: return y;
        case 1: return 256 - (y - 256);
        case 2: return -y;
        case 3: return -(256 - (y - 256));
    }
    return 0;
}

static inline int cos_approx(int angle) {
    return sin_approx(angle + 90);
}

static inline void gfx_draw_arc(int cx, int cy, int r, int start_angle, int end_angle, Color color) {
    for (int a = start_angle; a <= end_angle; a++) {
        int x = cx + (r * cos_approx(a)) / 256;
        int y = cy - (r * sin_approx(a)) / 256;
        gfx_put_pixel(x, y, color);
    }
}

// ---------- Bezier curve ----------
static inline void gfx_draw_bezier_quadratic(int x0, int y0, int x1, int y1, int x2, int y2, Color color) {
    int prev_x = x0, prev_y = y0;
    for (int t = 0; t <= 100; t += 2) {
        int t2 = t * t;
        int mt = 100 - t;
        int mt2 = mt * mt;
        
        int x = (mt2 * x0 + 2 * mt * t * x1 + t2 * x2) / 10000;
        int y = (mt2 * y0 + 2 * mt * t * y1 + t2 * y2) / 10000;
        
        gfx_draw_line(prev_x, prev_y, x, y, color);
        prev_x = x;
        prev_y = y;
    }
}

static inline void gfx_draw_bezier_cubic(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, Color color) {
    int prev_x = x0, prev_y = y0;
    for (int t = 0; t <= 100; t += 2) {
        int t2 = t * t;
        int t3 = t2 * t;
        int mt = 100 - t;
        int mt2 = mt * mt;
        int mt3 = mt2 * mt;
        
        int x = (mt3 * x0 + 3 * mt2 * t * x1 + 3 * mt * t2 * x2 + t3 * x3) / 1000000;
        int y = (mt3 * y0 + 3 * mt2 * t * y1 + 3 * mt * t2 * y2 + t3 * y3) / 1000000;
        
        gfx_draw_line(prev_x, prev_y, x, y, color);
        prev_x = x;
        prev_y = y;
    }
}

// ---------- Star shape ----------
static inline void gfx_draw_star(int cx, int cy, int r_outer, int r_inner, int points, Color color) {
    int angle_step = 360 / (points * 2);
    int prev_x = cx + (r_outer * cos_approx(0)) / 256;
    int prev_y = cy - (r_outer * sin_approx(0)) / 256;
    
    for (int i = 1; i <= points * 2; i++) {
        int angle = i * angle_step;
        int r = (i % 2 == 0) ? r_outer : r_inner;
        int x = cx + (r * cos_approx(angle)) / 256;
        int y = cy - (r * sin_approx(angle)) / 256;
        gfx_draw_line(prev_x, prev_y, x, y, color);
        prev_x = x;
        prev_y = y;
    }
}

static inline void gfx_fill_star(int cx, int cy, int r_outer, int r_inner, int points, Color color) {
    int angle_step = 360 / (points * 2);
    
    for (int i = 0; i < points * 2; i++) {
        int angle1 = i * angle_step;
        int angle2 = (i + 1) * angle_step;
        int r1 = (i % 2 == 0) ? r_outer : r_inner;
        int r2 = ((i + 1) % 2 == 0) ? r_outer : r_inner;
        
        int x1 = cx + (r1 * cos_approx(angle1)) / 256;
        int y1 = cy - (r1 * sin_approx(angle1)) / 256;
        int x2 = cx + (r2 * cos_approx(angle2)) / 256;
        int y2 = cy - (r2 * sin_approx(angle2)) / 256;
        
        gfx_fill_triangle(cx, cy, x1, y1, x2, y2, color);
    }
}

// ---------- Effects ----------
static inline void gfx_draw_glow(int cx, int cy, int r, Color color, int intensity) {
    for (int y = -r - intensity; y <= r + intensity; y++) {
        for (int x = -r - intensity; x <= r + intensity; x++) {
            int dist_sq = x * x + y * y;
            int dist = 0;
            for (int i = 1; i * i <= dist_sq; i++) dist = i;
            
            if (dist <= r) {
                gfx_put_pixel(cx + x, cy + y, color);
            } else if (dist <= r + intensity) {
                int fade = 255 - ((dist - r) * 255 / intensity);
                Color bg = gfx_get_pixel(cx + x, cy + y);
                gfx_put_pixel(cx + x, cy + y, color_blend(color, bg, fade));
            }
        }
    }
}

static inline void gfx_fill_checkerboard(int x, int y, int w, int h, int size, Color c1, Color c2) {
    for (int dy = 0; dy < h; dy++) {
        for (int dx = 0; dx < w; dx++) {
            int check = ((dx / size) + (dy / size)) % 2;
            gfx_put_pixel(x + dx, y + dy, check ? c1 : c2);
        }
    }
}

static inline void gfx_fill_noise(int x, int y, int w, int h, uint32_t seed) {
    for (int dy = 0; dy < h; dy++) {
        for (int dx = 0; dx < w; dx++) {
            seed = seed * 1103515245 + 12345;
            uint8_t gray = (seed >> 16) & 0xFF;
            gfx_put_pixel(x + dx, y + dy, RGB(gray, gray, gray));
        }
    }
}

#endif // GRAPHICS_H
