/*
 * func_800B9A08
 *
 * Purpose:
 *   Stores three 32-bit arguments into globals D_8010CD84, D_800F0F6C,
 *   and D_8010CD58.
 *
 * Arguments:
 *   arg0 (unsigned int): value stored to D_8010CD84
 *   arg1 (unsigned int): value stored to D_800F0F6C
 *   arg2 (unsigned int): value stored to D_8010CD58
 *
 * Return value:
 *   None (void)
 *
 * Global fields written:
 *   D_8010CD84 (u32)
 *   D_800F0F6C (u32)
 *   D_8010CD58 (u32)
 */

extern unsigned int D_8010CD84;
extern unsigned int D_800F0F6C;
extern unsigned int D_8010CD58;

void func_800B9A08(unsigned int arg0, unsigned int arg1, unsigned int arg2)
{
    D_8010CD84 = arg0;
    D_800F0F6C = arg1;
    D_8010CD58 = arg2;
}
