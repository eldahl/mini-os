BITS 16
global start
extern main

section .text

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    call main

.hang:
    hlt
    jmp .hang

