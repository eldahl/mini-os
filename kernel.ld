/* kernel.ld — used with ld.lld -T kernel.ld */
ENTRY(kernel_entry)

SECTIONS
{
  /* Kernel load & link address: matches boot.asm (0x00010000) */
  . = 0x00010000;

  .text :
  {
    *(.text*)
  }

  .rodata :
  {
    *(.rodata*)
  }

  .data :
  {
    *(.data*)
  }

  .bss :
  {
    *(.bss*)
    *(COMMON)
  }
}

