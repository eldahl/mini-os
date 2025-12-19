#ifndef PCI_H
#define PCI_H

#include <stdint.h>

// ============================================================================
// PCI BUS DRIVER FOR MINI-OS
// Provides PCI configuration space access and device enumeration
// ============================================================================

// PCI Configuration Space I/O Ports
#define PCI_CONFIG_ADDR  0xCF8
#define PCI_CONFIG_DATA  0xCFC

// PCI Configuration Space Registers (offsets)
#define PCI_VENDOR_ID        0x00
#define PCI_DEVICE_ID        0x02
#define PCI_COMMAND          0x04
#define PCI_STATUS           0x06
#define PCI_REVISION_ID      0x08
#define PCI_PROG_IF          0x09
#define PCI_SUBCLASS         0x0A
#define PCI_CLASS            0x0B
#define PCI_CACHE_LINE_SIZE  0x0C
#define PCI_LATENCY_TIMER    0x0D
#define PCI_HEADER_TYPE      0x0E
#define PCI_BIST             0x0F
#define PCI_BAR0             0x10
#define PCI_BAR1             0x14
#define PCI_BAR2             0x18
#define PCI_BAR3             0x1C
#define PCI_BAR4             0x20
#define PCI_BAR5             0x24
#define PCI_INTERRUPT_LINE   0x3C
#define PCI_INTERRUPT_PIN    0x3D

// PCI Command Register Bits
#define PCI_CMD_IO_SPACE     0x0001
#define PCI_CMD_MEM_SPACE    0x0002
#define PCI_CMD_BUS_MASTER   0x0004

// PCI Class Codes
#define PCI_CLASS_DISPLAY    0x03
#define PCI_SUBCLASS_VGA     0x00
#define PCI_SUBCLASS_3D      0x02

// Known GPU Vendor IDs
#define PCI_VENDOR_AMD       0x1002
#define PCI_VENDOR_NVIDIA    0x10DE
#define PCI_VENDOR_INTEL     0x8086
#define PCI_VENDOR_VMWARE    0x15AD
#define PCI_VENDOR_QEMU      0x1234
#define PCI_VENDOR_VIRTIO    0x1AF4
#define PCI_VENDOR_BOCHS     0x1234
#define PCI_VENDOR_REDHAT    0x1B36

// QEMU/Bochs Standard VGA Device
#define PCI_DEVICE_BOCHS_VGA 0x1111
#define PCI_DEVICE_QEMU_VGA  0x1111

// ---------- I/O Port Access ----------
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline void outl(uint16_t port, uint32_t val) {
    __asm__ volatile ("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    __asm__ volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// ---------- PCI Configuration Space Access ----------

// Build PCI config address
static inline uint32_t pci_config_addr(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    return (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | 0x80000000);
}

// Read 32-bit value from PCI config space
static inline uint32_t pci_read32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    outl(PCI_CONFIG_ADDR, pci_config_addr(bus, slot, func, offset));
    return inl(PCI_CONFIG_DATA);
}

// Read 16-bit value from PCI config space
static inline uint16_t pci_read16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    outl(PCI_CONFIG_ADDR, pci_config_addr(bus, slot, func, offset));
    return (uint16_t)(inl(PCI_CONFIG_DATA) >> ((offset & 2) * 8));
}

// Read 8-bit value from PCI config space
static inline uint8_t pci_read8(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    outl(PCI_CONFIG_ADDR, pci_config_addr(bus, slot, func, offset));
    return (uint8_t)(inl(PCI_CONFIG_DATA) >> ((offset & 3) * 8));
}

// Write 32-bit value to PCI config space
static inline void pci_write32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t val) {
    outl(PCI_CONFIG_ADDR, pci_config_addr(bus, slot, func, offset));
    outl(PCI_CONFIG_DATA, val);
}

// Write 16-bit value to PCI config space
static inline void pci_write16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t val) {
    outl(PCI_CONFIG_ADDR, pci_config_addr(bus, slot, func, offset));
    uint32_t tmp = inl(PCI_CONFIG_DATA);
    tmp &= ~(0xFFFF << ((offset & 2) * 8));
    tmp |= (uint32_t)val << ((offset & 2) * 8);
    outl(PCI_CONFIG_DATA, tmp);
}

// ---------- PCI Device Structure ----------
typedef struct {
    uint8_t  bus;
    uint8_t  slot;
    uint8_t  func;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t  class_code;
    uint8_t  subclass;
    uint8_t  prog_if;
    uint8_t  revision;
    uint8_t  header_type;
    uint8_t  interrupt_line;
    uint8_t  interrupt_pin;
    uint32_t bar[6];
    uint32_t bar_size[6];
    uint8_t  bar_type[6];  // 0=unused, 1=IO, 2=MEM32, 3=MEM64
} PCIDevice;

#define PCI_MAX_DEVICES 32
static PCIDevice pci_devices[PCI_MAX_DEVICES];
static int pci_device_count = 0;

// ---------- BAR Helpers ----------
#define PCI_BAR_IO        0x01
#define PCI_BAR_MEM       0x00
#define PCI_BAR_MEM_TYPE  0x06
#define PCI_BAR_MEM_32    0x00
#define PCI_BAR_MEM_64    0x04

static inline int pci_bar_is_io(uint32_t bar) {
    return bar & PCI_BAR_IO;
}

static inline uint32_t pci_bar_get_addr(uint32_t bar) {
    if (bar & PCI_BAR_IO) {
        return bar & 0xFFFFFFFC;
    } else {
        return bar & 0xFFFFFFF0;
    }
}

// Get BAR size - simplified version (skip the risky write)
static inline uint32_t pci_get_bar_size(uint8_t bus, uint8_t slot, uint8_t func, uint8_t bar_num) {
    (void)bus; (void)slot; (void)func; (void)bar_num;
    // Skip BAR size detection to avoid potential issues
    // Real drivers would probe this properly
    return 0;
}

// ---------- PCI Device Enumeration ----------
static inline int pci_check_device(uint8_t bus, uint8_t slot, uint8_t func) {
    uint16_t vendor = pci_read16(bus, slot, func, PCI_VENDOR_ID);
    if (vendor == 0xFFFF) return 0;
    
    if (pci_device_count >= PCI_MAX_DEVICES) return 0;
    
    PCIDevice* dev = &pci_devices[pci_device_count];
    dev->bus = bus;
    dev->slot = slot;
    dev->func = func;
    dev->vendor_id = vendor;
    dev->device_id = pci_read16(bus, slot, func, PCI_DEVICE_ID);
    dev->class_code = pci_read8(bus, slot, func, PCI_CLASS);
    dev->subclass = pci_read8(bus, slot, func, PCI_SUBCLASS);
    dev->prog_if = pci_read8(bus, slot, func, PCI_PROG_IF);
    dev->revision = pci_read8(bus, slot, func, PCI_REVISION_ID);
    dev->header_type = pci_read8(bus, slot, func, PCI_HEADER_TYPE);
    dev->interrupt_line = pci_read8(bus, slot, func, PCI_INTERRUPT_LINE);
    dev->interrupt_pin = pci_read8(bus, slot, func, PCI_INTERRUPT_PIN);
    
    // Read BARs (only for standard header type 0)
    if ((dev->header_type & 0x7F) == 0) {
        for (int i = 0; i < 6; i++) {
            dev->bar[i] = pci_read32(bus, slot, func, PCI_BAR0 + i * 4);
            dev->bar_size[i] = pci_get_bar_size(bus, slot, func, i);
            
            if (dev->bar[i] == 0) {
                dev->bar_type[i] = 0;
            } else if (dev->bar[i] & PCI_BAR_IO) {
                dev->bar_type[i] = 1;
            } else if ((dev->bar[i] & PCI_BAR_MEM_TYPE) == PCI_BAR_MEM_64) {
                dev->bar_type[i] = 3;
                i++; // Skip next BAR (upper 32 bits)
            } else {
                dev->bar_type[i] = 2;
            }
        }
    }
    
    pci_device_count++;
    return 1;
}

static inline void pci_enumerate(void) {
    pci_device_count = 0;
    
    // Only scan bus 0 for simplicity (covers most devices in QEMU)
    for (int slot = 0; slot < 32; slot++) {
        uint16_t vendor = pci_read16(0, slot, 0, PCI_VENDOR_ID);
        if (vendor == 0xFFFF || vendor == 0x0000) continue;
        
        pci_check_device(0, slot, 0);
        
        // Check for multi-function device
        uint8_t header = pci_read8(0, slot, 0, PCI_HEADER_TYPE);
        if (header & 0x80) {
            for (int func = 1; func < 8; func++) {
                if (pci_read16(0, slot, func, PCI_VENDOR_ID) != 0xFFFF) {
                    pci_check_device(0, slot, func);
                }
            }
        }
        
        if (pci_device_count >= PCI_MAX_DEVICES) break;
    }
}

// ---------- PCI Device Lookup ----------
static inline PCIDevice* pci_find_device(uint16_t vendor, uint16_t device) {
    for (int i = 0; i < pci_device_count; i++) {
        if (pci_devices[i].vendor_id == vendor && pci_devices[i].device_id == device) {
            return &pci_devices[i];
        }
    }
    return 0;
}

static inline PCIDevice* pci_find_class(uint8_t class_code, uint8_t subclass) {
    for (int i = 0; i < pci_device_count; i++) {
        if (pci_devices[i].class_code == class_code && pci_devices[i].subclass == subclass) {
            return &pci_devices[i];
        }
    }
    return 0;
}

static inline PCIDevice* pci_find_display(void) {
    // First try VGA controller
    PCIDevice* dev = pci_find_class(PCI_CLASS_DISPLAY, PCI_SUBCLASS_VGA);
    if (dev) return dev;
    // Then try 3D controller
    return pci_find_class(PCI_CLASS_DISPLAY, PCI_SUBCLASS_3D);
}

// ---------- PCI Device Enable ----------
static inline void pci_enable_device(PCIDevice* dev) {
    uint16_t cmd = pci_read16(dev->bus, dev->slot, dev->func, PCI_COMMAND);
    cmd |= PCI_CMD_IO_SPACE | PCI_CMD_MEM_SPACE | PCI_CMD_BUS_MASTER;
    pci_write16(dev->bus, dev->slot, dev->func, PCI_COMMAND, cmd);
}

// ---------- Vendor Name Lookup ----------
static inline const char* pci_vendor_name(uint16_t vendor) {
    switch (vendor) {
        case PCI_VENDOR_AMD:    return "AMD/ATI";
        case PCI_VENDOR_NVIDIA: return "NVIDIA";
        case PCI_VENDOR_INTEL:  return "Intel";
        case PCI_VENDOR_VMWARE: return "VMware";
        case PCI_VENDOR_QEMU:   return "QEMU/Bochs";
        case PCI_VENDOR_VIRTIO: return "VirtIO";
        case PCI_VENDOR_REDHAT: return "Red Hat";
        default:                return "Unknown";
    }
}

static inline const char* pci_class_name(uint8_t class_code, uint8_t subclass) {
    if (class_code == PCI_CLASS_DISPLAY) {
        switch (subclass) {
            case 0x00: return "VGA Controller";
            case 0x01: return "XGA Controller";
            case 0x02: return "3D Controller";
            default:   return "Display Controller";
        }
    }
    return "Other Device";
}

#endif // PCI_H

