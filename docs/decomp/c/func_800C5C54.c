/*
 * func_800C5C54
 *
 * Purpose:
 *   Copies a resident machine-code stub (from func_800C5AA8 + 0x10 up to
 *   D_800C5B28) into low RAM at 0x0000DF80 (PS1 kernel/BIOS patch region,
 *   also referenced via uncached KSEG1 0xA000DF80 in func_800C5AA8).
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Memory written:
 *   0x0000DF80 .. 0x0000DF80 + (D_800C5B28 - (func_800C5AA8 + 0x10))
 */

extern const unsigned int func_800C5AA8[];
extern const unsigned int D_800C5B28[];

void func_800C5C54(void)
{
    volatile unsigned int *dst = (volatile unsigned int *)0x0000DF80;
    const unsigned int *src = (const unsigned int *)((const unsigned char *)func_800C5AA8 + 0x10);
    const unsigned int *end = D_800C5B28;

    do {
        *dst++ = *src++;
    } while (src != end);
}
