#ifndef GPU_HW_H
#define GPU_HW_H

#include <stdint.h>
#include "pci.h"

// ============================================================================
// HARDWARE GPU DRIVER FOR MINI-OS
// Real GPU detection and basic hardware access via PCI
// ============================================================================

// ---------- GPU Hardware Types ----------
typedef enum {
    GPU_HW_NONE = 0,
    GPU_HW_VBE,           // VESA BIOS Extensions (fallback)
    GPU_HW_BOCHS,         // Bochs/QEMU VGA
    GPU_HW_VMWARE_SVGA,   // VMware SVGA
    GPU_HW_VIRTIO_GPU,    // VirtIO GPU
    GPU_HW_INTEL,         // Intel integrated
    GPU_HW_AMD,           // AMD/ATI
    GPU_HW_NVIDIA,        // NVIDIA
} GPUHardwareType;

// ---------- Bochs VGA / QEMU stdvga Registers ----------
// Bochs VBE Dispi interface (I/O ports)
#define VBE_DISPI_IOPORT_INDEX  0x01CE
#define VBE_DISPI_IOPORT_DATA   0x01CF

// Bochs VBE Dispi registers
#define VBE_DISPI_INDEX_ID          0x0
#define VBE_DISPI_INDEX_XRES        0x1
#define VBE_DISPI_INDEX_YRES        0x2
#define VBE_DISPI_INDEX_BPP         0x3
#define VBE_DISPI_INDEX_ENABLE      0x4
#define VBE_DISPI_INDEX_BANK        0x5
#define VBE_DISPI_INDEX_VIRT_WIDTH  0x6
#define VBE_DISPI_INDEX_VIRT_HEIGHT 0x7
#define VBE_DISPI_INDEX_X_OFFSET    0x8
#define VBE_DISPI_INDEX_Y_OFFSET    0x9
#define VBE_DISPI_INDEX_VIDEO_MEM   0xA

// Bochs VBE Dispi enable flags
#define VBE_DISPI_DISABLED      0x00
#define VBE_DISPI_ENABLED       0x01
#define VBE_DISPI_LFB_ENABLED   0x40
#define VBE_DISPI_NOCLEARMEM    0x80

// Bochs VBE ID values
#define VBE_DISPI_ID0           0xB0C0
#define VBE_DISPI_ID1           0xB0C1
#define VBE_DISPI_ID2           0xB0C2
#define VBE_DISPI_ID3           0xB0C3
#define VBE_DISPI_ID4           0xB0C4
#define VBE_DISPI_ID5           0xB0C5

// ---------- GPU Hardware State ----------
typedef struct {
    GPUHardwareType type;
    PCIDevice* pci_dev;
    
    // Framebuffer info
    uint32_t fb_addr;       // Physical address of framebuffer
    uint32_t fb_size;       // Size in bytes
    uint16_t width;
    uint16_t height;
    uint8_t  bpp;
    uint16_t pitch;
    
    // Memory-mapped registers (if applicable)
    uint32_t mmio_addr;
    uint32_t mmio_size;
    
    // I/O ports (if applicable)
    uint16_t io_base;
    
    // Capabilities
    uint8_t  has_accel;     // Hardware acceleration available
    uint8_t  has_cursor;    // Hardware cursor
    uint32_t vram_size;     // Total video RAM
} GPUHardware;

static GPUHardware g_gpu_hw;

// ---------- Bochs VGA I/O Access ----------
static inline void bochs_write(uint16_t index, uint16_t value) {
    outw(VBE_DISPI_IOPORT_INDEX, index);
    outw(VBE_DISPI_IOPORT_DATA, value);
}

static inline uint16_t bochs_read(uint16_t index) {
    outw(VBE_DISPI_IOPORT_INDEX, index);
    return inw(VBE_DISPI_IOPORT_DATA);
}

// ---------- Bochs VGA Detection ----------
static inline int bochs_detect(void) {
    uint16_t id = bochs_read(VBE_DISPI_INDEX_ID);
    return (id >= VBE_DISPI_ID0 && id <= VBE_DISPI_ID5);
}

static inline uint16_t bochs_get_version(void) {
    return bochs_read(VBE_DISPI_INDEX_ID);
}

static inline uint32_t bochs_get_vram_size(void) {
    // Read video memory size (in 64KB units for older versions)
    uint16_t mem = bochs_read(VBE_DISPI_INDEX_VIDEO_MEM);
    if (mem == 0) {
        // Fallback: assume 16MB
        return 16 * 1024 * 1024;
    }
    return (uint32_t)mem * 64 * 1024;
}

// ---------- Bochs VGA Mode Setting ----------
static inline int bochs_set_mode(uint16_t width, uint16_t height, uint8_t bpp) {
    // Disable VBE first
    bochs_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
    
    // Set resolution and BPP
    bochs_write(VBE_DISPI_INDEX_XRES, width);
    bochs_write(VBE_DISPI_INDEX_YRES, height);
    bochs_write(VBE_DISPI_INDEX_BPP, bpp);
    
    // Set virtual resolution (for scrolling/double buffering)
    bochs_write(VBE_DISPI_INDEX_VIRT_WIDTH, width);
    bochs_write(VBE_DISPI_INDEX_VIRT_HEIGHT, height * 2); // Double for page flipping
    
    // Set display offset
    bochs_write(VBE_DISPI_INDEX_X_OFFSET, 0);
    bochs_write(VBE_DISPI_INDEX_Y_OFFSET, 0);
    
    // Enable with linear framebuffer
    bochs_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED);
    
    return 0;
}

// Hardware page flip (for double buffering)
static inline void bochs_set_y_offset(uint16_t y_offset) {
    bochs_write(VBE_DISPI_INDEX_Y_OFFSET, y_offset);
}

// ---------- GPU Hardware Initialization ----------
static inline int gpu_hw_init(void) {
    // Initialize state
    g_gpu_hw.type = GPU_HW_NONE;
    g_gpu_hw.pci_dev = 0;
    g_gpu_hw.fb_addr = 0;
    g_gpu_hw.mmio_addr = 0;
    g_gpu_hw.has_accel = 0;
    g_gpu_hw.has_cursor = 0;
    
    // Enumerate PCI bus
    pci_enumerate();
    
    // Find display controller
    PCIDevice* gpu = pci_find_display();
    
    if (gpu) {
        g_gpu_hw.pci_dev = gpu;
        
        // Enable the device
        pci_enable_device(gpu);
        
        // Get framebuffer from BAR0 (typically)
        if (gpu->bar_type[0] == 2 || gpu->bar_type[0] == 3) {
            g_gpu_hw.fb_addr = pci_bar_get_addr(gpu->bar[0]);
            g_gpu_hw.fb_size = gpu->bar_size[0];
        }
        
        // Get MMIO from BAR2 (for some GPUs)
        if (gpu->bar_type[2] == 2) {
            g_gpu_hw.mmio_addr = pci_bar_get_addr(gpu->bar[2]);
            g_gpu_hw.mmio_size = gpu->bar_size[2];
        }
        
        // Detect GPU type
        if (gpu->vendor_id == PCI_VENDOR_QEMU || gpu->vendor_id == PCI_VENDOR_BOCHS) {
            if (bochs_detect()) {
                g_gpu_hw.type = GPU_HW_BOCHS;
                g_gpu_hw.vram_size = bochs_get_vram_size();
                g_gpu_hw.has_accel = 0;
                g_gpu_hw.has_cursor = 1;
            }
        } else if (gpu->vendor_id == PCI_VENDOR_VMWARE) {
            g_gpu_hw.type = GPU_HW_VMWARE_SVGA;
            // VMware SVGA has more complex initialization
        } else if (gpu->vendor_id == PCI_VENDOR_INTEL) {
            g_gpu_hw.type = GPU_HW_INTEL;
        } else if (gpu->vendor_id == PCI_VENDOR_AMD) {
            g_gpu_hw.type = GPU_HW_AMD;
        } else if (gpu->vendor_id == PCI_VENDOR_NVIDIA) {
            g_gpu_hw.type = GPU_HW_NVIDIA;
        }
        
        return 0;
    }
    
    // No PCI GPU found, check for Bochs VGA via I/O ports
    if (bochs_detect()) {
        g_gpu_hw.type = GPU_HW_BOCHS;
        g_gpu_hw.vram_size = bochs_get_vram_size();
        // Standard VGA framebuffer at 0xE0000000 or from VBE
        return 0;
    }
    
    // Fallback to VBE
    g_gpu_hw.type = GPU_HW_VBE;
    return -1;
}

// ---------- GPU Mode Setting ----------
static inline int gpu_hw_set_mode(uint16_t width, uint16_t height, uint8_t bpp) {
    if (g_gpu_hw.type == GPU_HW_BOCHS) {
        bochs_set_mode(width, height, bpp);
        
        g_gpu_hw.width = width;
        g_gpu_hw.height = height;
        g_gpu_hw.bpp = bpp;
        g_gpu_hw.pitch = width * (bpp / 8);
        
        return 0;
    }
    
    // For other GPU types, we rely on VBE mode set by bootloader
    return -1;
}

// ---------- GPU Page Flip (Hardware Double Buffering) ----------
static inline void gpu_hw_flip(int page) {
    if (g_gpu_hw.type == GPU_HW_BOCHS) {
        bochs_set_y_offset(page * g_gpu_hw.height);
    }
}

// ---------- GPU Info ----------
static inline const char* gpu_hw_type_string(void) {
    switch (g_gpu_hw.type) {
        case GPU_HW_VBE:        return "VBE (VESA)";
        case GPU_HW_BOCHS:      return "Bochs/QEMU VGA";
        case GPU_HW_VMWARE_SVGA: return "VMware SVGA";
        case GPU_HW_VIRTIO_GPU: return "VirtIO GPU";
        case GPU_HW_INTEL:      return "Intel GPU";
        case GPU_HW_AMD:        return "AMD/ATI GPU";
        case GPU_HW_NVIDIA:     return "NVIDIA GPU";
        default:                return "Unknown";
    }
}

static inline GPUHardware* gpu_hw_get_info(void) {
    return &g_gpu_hw;
}

#endif // GPU_HW_H

