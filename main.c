void putc(char c) {
    __asm__ volatile (
        ".intel_syntax noprefix\n"
        "mov ah, 0x0E\n"
        "int 0x10\n"
        ".att_syntax\n"
        :
        : "a"(c)
    );
}

void main(void) {
    const char *s = "Hello from Clang!\r\n";
    while (*s) putc(*s++);
    for (;;) { __asm__ volatile("hlt"); } /* never return */
}

