#!/usr/bin/env bash
set -euo pipefail

# Clean
rm -f *.o result.bin

# 1) Compile C -> ELF object (Clang)
clang --target=i386-elf -m16 -ffreestanding -fno-pie -nostdlib -c main.c -o main.o

# 2) Assemble bootloader -> ELF object (NASM)
nasm -f elf32 boot.asm -o boot.o

# 3) Link with your linker script into a flat binary (using linker.ld)
ld.lld -m elf_i386 -T linker.ld --oformat binary boot.o main.o -o result.bin

# 4) Sanity checks
echo "result.bin size: $(stat -c%s result.bin) bytes"
printf "tail of binary:\n"
hexdump -C result.bin | tail -n 6

# 5) Run as a floppy (BIOS will load sector to 0x7C00)
qemu-system-x86_64 -fda result.bin -nographic -boot order=a -net none

