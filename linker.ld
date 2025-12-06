ENTRY(start)

SECTIONS
{
  /* BIOS loads sector 0 to 0x7C00 */
  . = 0x7C00;

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
  }

  /* Force boot signature at byte 510 */
  . = 0x7C00 + 510;
  .boot :
  {
    SHORT(0xAA55);
  }
}

