; kernel_entry.asm — assembled with: nasm -f elf32 kernel_entry.asm -o kernel_entry.o
BITS 32

global kernel_entry
extern kmain

kernel_entry:
    ; Bootloader already set up segments and stack
    call kmain

.hang:
    cli
    hlt
    jmp .hang

