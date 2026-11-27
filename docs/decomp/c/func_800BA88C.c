/*
 * func_800BA88C (and trailing unlabelled function sub_800BA89C)
 *
 * Purpose:
 *   func_800BA88C: Returns the 16-bit unsigned global D_800D3DEE (inside-dispatcher flag).
 *
 *   sub_800BA89C (unlabelled function immediately following `jr $ra` in
 *   func_800BA88C.s): Dereferences the pointer D_800D4E7C and returns the
 *   16-bit unsigned value at *D_800D4E7C.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   func_800BA88C: unsigned short - value of D_800D3DEE
 *   sub_800BA89C:  unsigned short - value of *D_800D4E7C
 *
 * Global fields read:
 *   D_800D3DEE (unsigned short)
 *   D_800D4E7C (unsigned short *)
 */

extern unsigned short D_800D3DEE;
extern unsigned short *D_800D4E7C;

unsigned short func_800BA88C(void)
{
    return D_800D3DEE;
}

/* UNCLEAR: unlabelled second function following jr $ra at 0x800BA89C in func_800BA88C.s */
unsigned short sub_800BA89C(void)
{
    return *D_800D4E7C;
}
