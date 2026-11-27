/*
 * func_800B4878
 *
 * Purpose:
 *   Stops/clears PS1 Root Counter 1 (descriptor 0xF2000001, RCntCNT1) by
 *   calling func_800AD050(0xF2000001), which validates `(desc & 0xFFFF) < 3`,
 *   clears the 16-bit field at `D_800CEA48 + ((desc & 0xFFFF) << 4)` to 0,
 *   and returns 1 (or 0 if out of range).
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   (int) status returned by func_800AD050 (1 on valid descriptor)
 */

extern int func_800AD050(unsigned int rcnt_desc);

int func_800B4878(void)
{
    return func_800AD050(0xF2000001U);
}
