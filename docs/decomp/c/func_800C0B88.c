/*
 * func_800C0B88
 *
 * Purpose:
 *   Clears the 16-bit global D_800F0F70 to 0.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Globals written:
 *   D_800F0F70 (short): Reset to 0
 */

extern short D_800F0F70;

void func_800C0B88(void)
{
    D_800F0F70 = 0;
}
