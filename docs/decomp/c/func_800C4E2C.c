/*
 * func_800C4E2C
 *
 * Purpose:
 *   Atomically (single-read/write) swaps D_8010A40C with `val`, returning the
 *   previous value of D_8010A40C.
 *
 *   Note: The assembly file also contains an unreachable adjacent block at
 *   offset +0x14 (lines 7-11 of func_800C4E2C.s) after `jr $ra`, which
 *   performs the identical swap on D_8010A410 (`prev = D_8010A410;
 *   D_8010A410 = val; return prev;`).
 *
 * Arguments:
 *   val (unsigned int): new value to store in D_8010A40C
 *
 * Return value:
 *   (unsigned int) previous value of D_8010A40C
 *
 * Global fields read/written:
 *   D_8010A40C (u32, R/W)
 *   D_8010A410 (u32, R/W in unreachable trailing block)
 */

extern unsigned int D_8010A40C;

unsigned int func_800C4E2C(unsigned int val)
{
    unsigned int prev = D_8010A40C;
    D_8010A40C = val;
    return prev;
}
