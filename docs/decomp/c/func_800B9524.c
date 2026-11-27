/*
 * func_800B9524
 *
 * Purpose:
 *   Swaps D_800D3C98 with `val` and returns the previous value.
 *
 *   Note on asm_compact contents:
 *   Immediately after the `jr $ra; sw $a0, 0x30($v1)` of func_800B9524, the
 *   compact assembly file also contains an unlabeled function body (at +0x18
 *   into the file) and `alabel D_800B95CC` (a small wrapper calling
 *   func_800B9A28). Those trailing blocks are unreachable from func_800B9524
 *   and are preserved below in a clearly marked reference block.
 *
 * Arguments:
 *   val (u32) - new value to store in D_800D3C98
 *
 * Return value:
 *   u32 - previous value of D_800D3C98
 *
 * Globals read/written:
 *   D_800D3C98 (u32, R/W) - callback or state word swapped by func_800B9524
 */

extern unsigned int D_800D3C98;

unsigned int func_800B9524(unsigned int val)
{
    unsigned int prev = D_800D3C98;
    D_800D3C98 = val;
    return prev;
}

/*
 * UNCLEAR: Unlabeled function body and `alabel D_800B95CC` included in
 * func_800B9524.s after func_800B9524's `jr $ra` (lines 8-50):
 *
 * extern unsigned int D_800E81D8;
 * extern int func_800B7034(unsigned int cmd, unsigned char *param, unsigned char *result);
 * extern void func_800B7450(void *cb);
 * extern void func_800B7020(void *cb);
 * extern void func_800B96D8(void);
 * extern void func_800B9A28(void);
 *
 * static void sub_trailing_in_func_800B9524(unsigned int modeFlags)
 * {
 *     unsigned char param[4];
 *     param[0] = (unsigned char)modeFlags;
 *     func_800B7034(0xE, param, 0); // CdlSetmode
 *     if ((modeFlags & 0x100U) != 0) {
 *         if ((modeFlags & 0x20U) != 0) {
 *             D_800E81D8 = 0;
 *         } else {
 *             D_800E81D8 = 1;
 *         }
 *         func_800B7450(func_800B96D8);
 *         func_800B7020(D_800B95CC);
 *     }
 *     func_800B7034(0x1B, 0, 0);
 * }
 *
 * static void D_800B95CC(void)
 * {
 *     func_800B9A28();
 * }
 */
