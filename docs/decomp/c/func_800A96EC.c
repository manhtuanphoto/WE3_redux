/*
 * func_800A96EC
 *
 * Purpose:
 *   Sets the 16-bit global D_800CE244 to 0xCA (202).
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Global fields written:
 *   D_800CE244 (unsigned short): set to 0xCA
 */

extern unsigned short D_800CE244;

void func_800A96EC(void)
{
    /* UNCLEAR: semantic meaning of D_800CE244 and constant 0xCA */
    D_800CE244 = 0xCAU;
}
