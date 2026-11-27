/*
 * func_800AE388
 *
 * Purpose:
 *   PsyQ-style GPU display/graph mode toggle (analogous to SetGraphDebug / display mask).
 *   If debug verbosity D_800CEAC5[1] (D_800CEAC6) >= 2, invokes debug callback
 *   D_800CEAC0(D_80014EC0, mode). If `mode` differs from current D_800CEAC5[0],
 *   calls (*(D_800CEABC + 0x34))(1), updates D_800CEAC5[0] = (u8)mode, and calls
 *   func_800BA798(2, 0). Returns the previous value of D_800CEAC5[0].
 *
 * Arguments:
 *   mode (signed int): target mode/state value
 *
 * Return value:
 *   signed int: previous value of D_800CEAC5[0]
 *
 * Globals read / written:
 *   D_80014EC0 (char[], R): debug/format string passed to D_800CEAC0
 *   D_800CEABC (u32, R): GPU control struct pointer (+0x34 function pointer)
 *   D_800CEAC0 (void (*)(void *, signed int), R): debug/error callback
 *   D_800CEAC5 (u8[], R/W): +0x00 current mode byte, +0x01 debug verbosity level (D_800CEAC6)
 */

extern char D_80014EC0[];
extern unsigned int D_800CEABC;
extern void (*D_800CEAC0)(void *, signed int);
extern unsigned char D_800CEAC5[];

extern void func_800BA798(signed int arg0, signed int arg1);

signed int func_800AE388(signed int mode)
{
    unsigned char prev = D_800CEAC5[0];

    if (D_800CEAC5[1] >= 2) {
        D_800CEAC0(D_80014EC0, mode);
    }

    if ((unsigned int)mode != (unsigned int)D_800CEAC5[0]) {
        (*(void (**)(signed int))(D_800CEABC + 0x34))(1);
        D_800CEAC5[0] = (unsigned char)mode;
        func_800BA798(2, 0);
    }

    return (signed int)prev;
}
