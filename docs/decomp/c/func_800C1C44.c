/*
 * func_800C1C44
 *
 * Purpose:
 *   Writes a 16-bit value into the SPU register / address table D_800D558C[slot].
 *   When `shiftFlag == 0`, writes `val` directly; when `shiftFlag != 0`, shifts
 *   `val` right by `D_800D55B4` (e.g., 3 for 8-byte SPU units) before storing.
 *
 * Arguments:
 *   slot      (int):          halfword index into D_800D558C
 *   val       (unsigned int): value or byte address to write
 *   shiftFlag (int):          0 = store raw `val`, non-zero = store `val >> D_800D55B4`
 *
 * Return value:
 *   None (void)
 *
 * Global fields read/written:
 *   D_800D558C (volatile unsigned short *): SPU MMIO / block table
 *   D_800D55B4 (unsigned int):              right-shift count when shiftFlag != 0
 */

extern volatile unsigned short *D_800D558C;
extern unsigned int D_800D55B4;

void func_800C1C44(int slot, unsigned int val, int shiftFlag)
{
    if (shiftFlag == 0) {
        D_800D558C[slot] = (unsigned short)val;
    } else {
        D_800D558C[slot] = (unsigned short)(val >> D_800D55B4);
    }
}
