/*
 * func_800ADB8C
 *
 * Purpose:
 *   Splits a 16-bit packed parameter into a 6-bit low field (scaled by 16)
 *   and a 10-bit high field, then invokes the callback stored in D_800CEAC0
 *   with D_80014D74 as the first argument.
 *
 * Arguments:
 *   arg0 (unsigned int): Packed value (low 6 bits -> arg1 * 16, bits 6..15 -> arg2)
 *
 * Return value:
 *   int: Return value from D_800CEAC0 callback (or void if caller ignores it)
 *
 * Globals read:
 *   D_80014D74 (buffer/struct/format address passed as arg0 to callback)
 *   D_800CEAC0 (function pointer)
 */

extern unsigned char D_80014D74[];
extern int (*D_800CEAC0)(void *arg0, unsigned int arg1, unsigned int arg2);

int func_800ADB8C(unsigned int arg0)
{
    /* UNCLEAR: exact semantics of D_80014D74 and callback D_800CEAC0 */
    unsigned int low = (arg0 & 0x3FU) << 4;
    unsigned int high = (arg0 & 0xFFFFU) >> 6;

    return D_800CEAC0(D_80014D74, low, high);
}
