/*
 * func_800B5238
 *
 * Purpose:
 *   Setter for the 32-bit global D_8010C29C.
 *
 * Arguments:
 *   arg0 (int): Value stored into D_8010C29C
 *
 * Return value:
 *   None (void)
 *
 * Globals written:
 *   D_8010C29C (int): Set to arg0
 */

extern int D_8010C29C;

void func_800B5238(int arg0)
{
    D_8010C29C = arg0;
}
