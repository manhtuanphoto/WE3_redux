/*
 * func_800B53F8
 *
 * Purpose:
 *   Writes PS1 GTE (COP2) control register $26 (H: projection plane distance).
 *   Matches PsyQ libgte SetGeomScreen(long h).
 *
 * Arguments:
 *   h (signed int) - projection plane distance written via `ctc2 $a0, $26`
 *
 * Return value:
 *   None (void)
 */

void func_800B53F8(signed int h)
{
    /* GTE (COP2) control register $26 = H (screen/projection distance) */
    __asm__ volatile("ctc2 %0, $26" : : "r"(h));
}
