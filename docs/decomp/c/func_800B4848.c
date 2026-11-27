/*
 * func_800B4848
 *
 * Purpose:
 *   Calls func_800ACFB4(0xF2000001), where 0xF2000001 is the PS1 BIOS
 *   Root Counter 1 descriptor (RCntCNT1).
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   (int) return value forwarded from func_800ACFB4
 *   UNCLEAR: func_800ACFB4 is not in asm_compact; exact return type
 *   (likely int/status or void) and internal behavior are unverified.
 */

extern int func_800ACFB4(unsigned int rcnt_desc);

int func_800B4848(void)
{
    return func_800ACFB4(0xF2000001U);
}
