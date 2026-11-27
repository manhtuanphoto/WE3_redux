/*
 * func_800B4958
 *
 * Purpose:
 *   Sets the base screen X and Y origin offsets in D_800EF868[0..1].
 *
 * Arguments:
 *   x (signed short): base screen X offset stored at D_800EF868 + 0x0
 *   y (signed short): base screen Y offset stored at D_800EF868 + 0x2
 *
 * Return value:
 *   None (void)
 *
 * Global fields written:
 *   D_800EF868 (signed short[2]): base screen X, Y origin offsets
 */

extern signed short D_800EF868[];

void func_800B4958(signed short x, signed short y)
{
    D_800EF868[0] = x;
    D_800EF868[1] = y;
}
