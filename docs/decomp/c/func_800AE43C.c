/*
 * func_800AE43C
 *
 * Purpose:
 *   Updates the 32-bit global D_800CEAD0 (located at &D_800CEAC6 + 0xA) and
 *   returns its previous value. When D_800CEAC6 >= 2, logs the new value via
 *   the callback pointer D_800CEAC0 with format string D_80014ED4.
 *
 * Arguments:
 *   val (unsigned int): new 32-bit value to store in D_800CEAD0
 *
 * Return value:
 *   unsigned int: previous 32-bit value of D_800CEAD0
 *
 * Global fields read/written:
 *   D_800CEAC0 (fn ptr): debug log/print callback
 *   D_800CEAC6 (u8):     verbosity/debug threshold byte (logs when >= 2)
 *   D_800CEAD0 (u32):    &D_800CEAC6 + 0xA, read and updated
 *   D_80014ED4 (u8[]):   format string passed as arg0
 */

#define W(p, o) (*(unsigned int *)((p) + (o)))

extern void (*D_800CEAC0)(unsigned char *fmt, unsigned int val);
extern unsigned char D_800CEAC6;
extern unsigned char D_80014ED4[];

unsigned int func_800AE43C(unsigned int val)
{
    unsigned char *base = &D_800CEAC6;
    unsigned int prev;

    if (D_800CEAC6 >= 2) {
        D_800CEAC0(D_80014ED4, val);
    }

    prev = W(base, 0xA);
    W(base, 0xA) = val;
    return prev;
}
