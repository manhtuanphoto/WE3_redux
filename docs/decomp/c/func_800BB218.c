/*
 * func_800BB218
 *
 * Purpose:
 *   Sets the 32-bit global D_800D4EF4 to `val` and returns its previous value.
 *   (Companion setter to getter func_800BB22C.)
 *
 * Arguments:
 *   val (unsigned int): new value to store in D_800D4EF4
 *
 * Return value:
 *   unsigned int: previous value of D_800D4EF4
 *
 * Global fields read/written:
 *   D_800D4EF4 (unsigned int)
 */

extern unsigned int D_800D4EF4;

unsigned int func_800BB218(unsigned int val)
{
    unsigned int prev = D_800D4EF4;
    D_800D4EF4 = val;
    return prev;
}
