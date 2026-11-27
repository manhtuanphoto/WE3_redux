/*
 * func_800AA7B4
 *
 * Purpose:
 *   Sets the gp-relative 32-bit global D_800E6B94 to 1 and returns 1.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   int: Always returns 1 ($v0 = 1)
 *
 * Globals written:
 *   D_800E6B94 (int): Set to 1
 */

extern int D_800E6B94;

int func_800AA7B4(void)
{
    D_800E6B94 = 1;
    return 1;
}
