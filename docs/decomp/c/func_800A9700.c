/*
 * func_800A9700
 *
 * Purpose:
 *   Sets the 16-bit global D_800CE244 to 0xB6 (182).
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Global fields written:
 *   D_800CE244 (unsigned short): set to 0xB6
 */

extern unsigned short D_800CE244;

void func_800A9700(void)
{
    /* UNCLEAR: semantic meaning of D_800CE244 and constant 0xB6 */
    D_800CE244 = 0xB6U;
}
