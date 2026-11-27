/*
 * func_800C3F88
 *
 * Purpose:
 *   Sets the SPU DMA transfer-completed flag D_800D55C0 based on `mode`:
 *   if `mode == 1`, clears D_800D55C0 to 0; otherwise sets D_800D55C0 to 1.
 *   (Note: D_800D55C0 is the cached transfer-completed flag checked in
 *   func_800C3ED8.)
 *
 * Arguments:
 *   mode (int): when 1, clears D_800D55C0 to 0; otherwise sets it to 1
 *
 * Return value:
 *   None (void) - $v0 holds 1 as a scratch constant used for comparison/store.
 *   UNCLEAR: whether any caller reads $v0 (1) or treats return as void.
 *
 * Global fields written:
 *   D_800D55C0 (u32)
 */

extern unsigned int D_800D55C0;

void func_800C3F88(int mode)
{
    if (mode == 1) {
        D_800D55C0 = 0U;
    } else {
        D_800D55C0 = 1U;
    }
}
