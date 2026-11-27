/*
 * func_800AE49C
 *
 * Purpose:
 *   PsyQ-style GPU display/drawing synchronization or mask control. If debug
 *   level D_800CEAC6[0] >= 2, calls debug callback D_800CEAC0(D_80014EF0, mode).
 *   When mode == 0, fills the 0x14-byte region at D_800CEAC6 + 0x6A (which
 *   corresponds to D_800CEAC4 + 0x6C) with -1 via func_800B10D8. Finally invokes
 *   (*(D_800CEABC + 0x10))(mode == 0 ? 0x03000001 : 0x03000000) and returns its result.
 *
 * Arguments:
 *   mode (signed int): mode/sync parameter (0 vs non-zero)
 *
 * Return value:
 *   signed int: return value of (*(D_800CEABC + 0x10))(...)
 *
 * Globals read / written:
 *   D_80014EF0 (char[], R): debug/format string passed to D_800CEAC0
 *   D_800CEABC (u32, R): GPU control struct pointer (+0x10 function pointer)
 *   D_800CEAC0 (void (*)(void *, signed int), R): debug/error callback
 *   D_800CEAC6 (u8[], R/W): +0x00 debug level, +0x6A 20-byte state table (D_800CEAC4 + 0x6C)
 */

extern char D_80014EF0[];
extern unsigned int D_800CEABC;
extern void (*D_800CEAC0)(void *, signed int);
extern unsigned char D_800CEAC6[];

extern void func_800B10D8(void *dst, signed int val, signed int size);

signed int func_800AE49C(signed int mode)
{
    unsigned int cmd;

    if (D_800CEAC6[0] >= 2) {
        D_800CEAC0(D_80014EF0, mode);
    }

    if (mode == 0) {
        func_800B10D8(D_800CEAC6 + 0x6A, -1, 0x14);
        cmd = 0x03000001U;
    } else {
        cmd = 0x03000000U;
    }

    return (*(signed int (**)(unsigned int))(D_800CEABC + 0x10))(cmd);
}
