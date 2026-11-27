/*
 * func_800C1188
 *
 * Purpose:
 *   16-bit signed wrapper around func_800C3ED8(flag) (PsyQ
 *   `SpuIsTransferCompleted` equivalent; analogous to `SsVabTransCompleted`).
 *   Sign-extends `flag` to 16 bits, calls func_800C3ED8, and sign-extends
 *   the return value to `signed short`.
 *
 * Arguments:
 *   flag (signed short): 1 = blocking wait, else non-blocking poll
 *
 * Return value:
 *   (signed short) 1 if transfer completed, or poll status from func_800C3ED8
 */

extern int func_800C3ED8(int flag);

signed short func_800C1188(signed short flag)
{
    return (signed short)func_800C3ED8((int)(signed short)flag);
}
