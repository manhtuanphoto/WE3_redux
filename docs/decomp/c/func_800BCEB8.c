/*
 * func_800BCEB8
 *
 * Purpose:
 *   Unpacks four fields from two 32-bit packed arguments and forwards them to
 *   func_800BF450().
 *
 * Arguments:
 *   packed0 (unsigned int): Bits 8..23 (sign-extended to s16) -> arg0 of func_800BF450;
 *                           Bits 0..7 (u8) -> arg1 of func_800BF450
 *   packed1 (unsigned int): Bits 8..23 (u16) -> arg2 of func_800BF450;
 *                           Bits 0..7 (u8) -> arg3 of func_800BF450 (unused by func_800BF450)
 *
 * Return value:
 *   int: Return value of func_800BF450()
 */

extern int func_800BF450(int arg0, int arg1, unsigned int arg2, unsigned int arg3);

int func_800BCEB8(unsigned int packed0, unsigned int packed1)
{
    int p0 = ((int)(packed0 << 8)) >> 16; /* signed 16-bit bits 8..23 of packed0 */
    int p1 = (int)(packed0 & 0xFFU);
    unsigned int p2 = (packed1 >> 8) & 0xFFFFU;
    unsigned int p3 = packed1 & 0xFFU;

    return func_800BF450(p0, p1, p2, p3);
}
