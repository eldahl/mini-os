#!/usr/bin/env bash
set -euo pipefail

echo "[1] Build kernel (32-bit ELF)..."
clang --target=i386-elf -m32 -ffreestanding -fno-pie -nostdlib -c kernel.c -o kernel.o
nasm -f elf32 kernel_entry.asm -o kernel_entry.o
ld.lld -m elf_i386 -T kernel.ld kernel_entry.o kernel.o -o kernel.elf

echo "[2] Convert kernel ELF -> flat binary..."
if command -v llvm-objcopy >/dev/null 2>&1; then
  llvm-objcopy -O binary kernel.elf kernel.bin
else
  objcopy -O binary kernel.elf kernel.bin
fi

echo "[3] Build boot sector..."
nasm -f bin boot.asm -o boot.bin

echo "[4] Create floppy disk image..."
dd if=/dev/zero of=disk.img bs=512 count=2880 status=none
dd if=boot.bin   of=disk.img conv=notrunc
dd if=kernel.bin of=disk.img bs=512 seek=1 conv=notrunc

echo "[5] Run in QEMU..."
qemu-system-x86_64 \
	-drive file=disk.img,format=raw,if=floppy \
	-boot order=a \
  -net none

