; boot.asm — 16-bit boot sector (must fit in 512 bytes!)
BITS 16
ORG 0x7C00

BOOTINFO equ 0x7E00
VBEMODE  equ 0x118
VBEBUF   equ 0x9000

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    mov [drv], dl

    ; Zero bootinfo
    mov di, BOOTINFO
    mov cx, 16
    rep stosw
    mov dword [BOOTINFO], 0x1BADB002
    mov al, [drv]
    mov [BOOTINFO+4], al

    ; VBE get mode info
    mov ax, VBEBUF
    mov es, ax
    xor di, di
    mov ax, 0x4F01
    mov cx, VBEMODE
    int 0x10
    cmp ax, 0x004F
    jne er

    ; VBE set mode
    mov ax, 0x4F02
    mov bx, VBEMODE | 0x4000
    int 0x10
    cmp ax, 0x004F
    jne er

    ; Copy VBE info
    mov eax, [es:0x28]
    mov [BOOTINFO+16], eax
    mov ax, [es:0x10]
    mov [BOOTINFO+20], ax
    mov ax, [es:0x12]
    mov [BOOTINFO+22], ax
    mov ax, [es:0x14]
    mov [BOOTINFO+24], ax
    mov al, [es:0x19]
    mov [BOOTINFO+26], al
    mov byte [BOOTINFO+27], 1

    ; Load kernel
    mov ax, 0x1000
    mov es, ax
    xor bx, bx
    mov ah, 2
    mov al, 60
    xor ch, ch
    mov cl, 2
    xor dh, dh
    mov dl, [drv]
    int 0x13
    jc er

    mov dword [BOOTINFO+8], 0x10000
    mov dword [BOOTINFO+12], 60

    ; Protected mode
    cli
    lgdt [gdt_desc]
    mov eax, cr0
    or al, 1
    mov cr0, eax
    jmp 0x08:pm

er: mov al, 'E'
    mov ah, 0x0E
    int 0x10
    hlt
    jmp er

drv: db 0
gdt: dq 0, 0x00CF9A000000FFFF, 0x00CF92000000FFFF
gdt_desc: dw 23
          dd gdt

BITS 32
pm: mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax
    mov esp, 0x90000
    call 0x10000
.h: hlt
    jmp .h

times 510-($-$$) db 0
dw 0xAA55
