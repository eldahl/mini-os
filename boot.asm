; boot.asm — 16-bit boot sector
; assemble with: nasm -f bin boot.asm -o boot.bin

BITS 16
ORG 0x7C00

BOOTINFO_ADDR equ 0x7E00
VBE_MODE      equ 0x118          ; 1024x768x16bpp (commonly available)
VBE_LINEAR    equ 0x4000
VBE_BUF_SEG   equ 0x9000         ; buffer for VBE mode info (below 64K)

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    ; -------------------------------------------------
    ; Initialize BootInfo struct at 0x7E00
    ; struct BootInfo {
    ;   uint32_t magic;
    ;   uint8_t  boot_drive;
    ;   uint8_t  _pad[3];
    ;   uint32_t kernel_phys;
    ;   uint32_t kernel_sectors;
    ;   uint32_t fb_addr;
    ;   uint16_t fb_pitch;
    ;   uint16_t fb_width;
    ;   uint16_t fb_height;
    ;   uint8_t  fb_bpp;
    ;   uint8_t  fb_type;
    ; }
    ; -------------------------------------------------
    mov di, BOOTINFO_ADDR
    mov cx, 16          ; zero 32 bytes (16 words)
    xor ax, ax
rep stosw               ; zero 32 bytes

    ; magic = 0x1BADB002
    mov dword [BOOTINFO_ADDR], 0x1BADB002

    ; boot_drive = DL
    mov byte [BOOTINFO_ADDR + 4], dl

    ; -------------------------------------------------
    ; Set VBE video mode with linear framebuffer
    ; -------------------------------------------------
    mov ax, VBE_BUF_SEG
    mov es, ax
    xor di, di

    mov ax, 0x4F01          ; VBE: get mode info
    mov cx, VBE_MODE
    int 0x10
    cmp ax, 0x004F
    jne vbe_fail

    ; set video mode (with linear framebuffer bit)
    mov ax, 0x4F02
    mov bx, VBE_MODE | VBE_LINEAR
    int 0x10
    cmp ax, 0x004F
    jne vbe_fail

    ; Copy VBE info into BootInfo
    ; fb_addr (dword at offset 0x28)
    mov eax, [es:0x28]
    mov [BOOTINFO_ADDR + 16], eax

    ; fb_pitch (word at offset 0x10)
    mov ax, [es:0x10]
    mov [BOOTINFO_ADDR + 20], ax

    ; fb_width (word at offset 0x12), fb_height (word at offset 0x14)
    mov ax, [es:0x12]
    mov [BOOTINFO_ADDR + 22], ax
    mov ax, [es:0x14]
    mov [BOOTINFO_ADDR + 24], ax

    ; fb_bpp (byte at offset 0x19)
    mov al, [es:0x19]
    mov [BOOTINFO_ADDR + 26], al

    ; fb_type: 1 = RGB
    mov byte [BOOTINFO_ADDR + 27], 1

    ; -------------------------------------------------
    ; Load kernel: 20 sectors from LBA sector 2
    ; into 0x1000:0000 (phys 0x00010000)
    ; -------------------------------------------------
    mov bx, 0x0000        ; offset
    mov ax, 0x1000        ; segment
    mov es, ax

    mov ah, 0x02          ; BIOS: read sectors
    mov al, 20            ; number of sectors
    mov ch, 0             ; cylinder 0
    mov dh, 0             ; head 0
    mov cl, 2             ; starting at sector 2
    mov dl, [BOOTINFO_ADDR + 4]  ; boot_drive we saved above

    int 0x13
    jc disk_error

    ; Save kernel info into BootInfo
    mov dword [BOOTINFO_ADDR + 8], 0x00010000  ; kernel_phys
    mov dword [BOOTINFO_ADDR + 12], 20         ; kernel_sectors

    ; -------------------------------------------------
    ; Set up GDT & enter protected mode
    ; -------------------------------------------------
    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 1             ; set PE bit
    mov cr0, eax

    ; Far jump to flush pipeline & load CS (code segment)
    jmp 0x08:pm_entry

; -------------------------------------------------
; 32-bit code starts here
; -------------------------------------------------
BITS 32
pm_entry:
    ; Set data segments
    mov ax, 0x10          ; data segment selector
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax

    ; Set up a stack
    mov esp, 0x00900000   ; just "somewhere" high in low memory

    ; Kernel entry is at physical 0x00010000
    mov eax, 0x00010000
    call eax              ; call kernel_entry (first instruction in kernel.bin)

pm_halt:
    cli
    hlt
    jmp pm_halt

; -------------------------------------------------
; Back to 16-bit for data / GDT / messages
; -------------------------------------------------
BITS 16

disk_error:
    mov si, disk_error_msg
.de_loop:
    lodsb
    or al, al
    jz .halt
    mov ah, 0x0E
    mov bh, 0
    mov bl, 0x0C
    int 0x10
    jmp .de_loop
.halt:
    cli
    hlt
    jmp .halt

boot_drive      db 0
disk_error_msg  db 'Disk read error',0
vbe_error_msg   db 'VBE init error',0

; ---------------- GDT ----------------

gdt_start:
    dq 0                      ; null descriptor

    ; Code segment: base=0, limit=4GB, RX
    dq 0x00CF9A000000FFFF

    ; Data segment: base=0, limit=4GB, RW
    dq 0x00CF92000000FFFF
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

; -------------------------------------------------
; Boot signature
; -------------------------------------------------
times 510-($-$$) db 0
dw 0xAA55

; -------------------------------------------------
; VBE failure handler (loops printing message)
; -------------------------------------------------
vbe_fail:
    mov si, vbe_error_msg
.vf_loop:
    lodsb
    or al, al
    jz .vf_halt
    mov ah, 0x0E
    mov bh, 0
    mov bl, 0x0C
    int 0x10
    jmp .vf_loop
.vf_halt:
    cli
    hlt
    jmp .vf_halt
