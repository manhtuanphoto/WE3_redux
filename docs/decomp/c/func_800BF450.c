/*
 * func_800BF450
 *
 * Purpose:
 *   Wrapper that sign-extends arg0 and arg1 to 16 bits, zero-extends arg2 to
 *   16 bits, and calls func_800BF1FC(0x21, (s16)arg0, (s16)arg1, (u16)arg2).
 *   (Note: incoming $a3 is unused and overwritten with arg2 & 0xFFFF; assembly
 *   also ends with a redundant duplicate `jr $ra`.)
 *
 * Arguments:
 *   arg0 (short): Sign-extended to 32-bit int and passed as param1 to func_800BF1FC
 *   arg1 (short): Sign-extended to 32-bit int and passed as param2 to func_800BF1FC
 *   arg2 (unsigned short): Masked to 0xFFFF and passed as param3 to func_800BF1FC
 *   arg3 (unsigned int): Unused (overwritten before jal func_800BF1FC)
 *
 * Return value:
 *   int: Return value from func_800BF1FC()
 */

extern int func_800BF1FC(int arg0, int arg1, int arg2, unsigned int arg3);

int func_800BF450(short arg0, short arg1, unsigned short arg2, unsigned int arg3)
{
    (void)arg3;
    return func_800BF1FC(0x21, (int)arg0, (int)arg1, (unsigned int)(arg2 & 0xFFFFU));
}
