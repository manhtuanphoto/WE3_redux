/*
 * func_800AE32C
 *
 * Purpose:
 *   Updates the unsigned byte state/verbosity flag D_800CEAC6 and returns its
 *   previous value. If the new byte value is non-zero, logs it alongside
 *   adjacent bytes D_800CEAC4 and D_800CEAC7 via the callback pointer D_800CEAC0
 *   with format/message D_80014E94.
 *
 * Arguments:
 *   val (unsigned int): new flag byte value (masked to 0xFF when tested)
 *
 * Return value:
 *   unsigned int: previous unsigned byte value of D_800CEAC6
 *
 * Global fields read/written:
 *   D_800CEAC0 (fn ptr): debug log/print callback
 *   D_800CEAC4 (u8):     byte at &D_800CEAC6 - 2, passed as arg2
 *   D_800CEAC6 (u8):     flag byte read and updated
 *   D_800CEAC7 (u8):     byte at &D_800CEAC6 + 1, passed as arg3
 *   D_80014E94 (u8[]):   format string / message passed as arg0
 */

extern void (*D_800CEAC0)(unsigned char *fmt, unsigned int a1, unsigned int a2, unsigned int a3);
extern unsigned char D_800CEAC4;
extern unsigned char D_800CEAC6;
extern unsigned char D_800CEAC7;
extern unsigned char D_80014E94[];

unsigned int func_800AE32C(unsigned int val)
{
    unsigned int prev = D_800CEAC6;

    D_800CEAC6 = (unsigned char)val;
    if ((val & 0xFFu) != 0) {
        D_800CEAC0(D_80014E94, D_800CEAC6, D_800CEAC4, D_800CEAC7);
    }

    return prev;
}
