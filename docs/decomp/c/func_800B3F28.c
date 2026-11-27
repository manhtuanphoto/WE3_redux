/*
 * func_800B3F28
 *
 * Purpose:
 *   Reads PS1 GTE (COP2) control register $26 (H: projection plane distance).
 *   Matches PsyQ libgte ReadGeomScreen().
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   signed int - current GTE H register value from `cfc2 $v0, $26`
 */

signed int func_800B3F28(void)
{
    signed int h;

    /* GTE (COP2) control register $26 = H (screen/projection distance) */
    __asm__ volatile("cfc2 %0, $26" : "=r"(h));
    return h;
}
