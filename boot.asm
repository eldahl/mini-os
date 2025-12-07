; boot.asm — 16-bit boot sector
; assemble with: nasm -f bin boot.asm -o boot.bin

BITS 16
ORG 0x7C00

BOOTINFO_ADDR equ 0x7E00

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
    ; }
    ; -------------------------------------------------
    mov di, BOOTINFO_ADDR
    mov cx, 12          ; 3 x 4-byte words = 12 bytes
    xor ax, ax
rep stosw               ; zero 24 bytes (actually 2*cx = 24 bytes)

    ; magic = 0x1BADB002
    mov dword [BOOTINFO_ADDR], 0x1BADB002

    ; boot_drive = DL
    mov byte [BOOTINFO_ADDR + 4], dl

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

