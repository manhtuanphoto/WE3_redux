/*
 * func_800BE498
 *
 * Purpose:
 *   Clears the 16-bit global D_800EF980 to 0.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Globals written:
 *   D_800EF980 (short): Reset to 0
 */

extern short D_800EF980;

void func_800BE498(void)
{
    D_800EF980 = 0;
}
