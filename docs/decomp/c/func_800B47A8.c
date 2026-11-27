/*
 * func_800B47A8
 *
 * Purpose:
 *   Sets the PS1 GTE screen geometry offset registers (PsyQ SetGeomOffset).
 *   Shifts `ofx` and `ofy` left by 16 bits into 16.16 fixed-point format and
 *   writes them to GTE control registers $24 (OFX) and $25 (OFY) via `ctc2`.
 *
 * Arguments:
 *   ofx (int): screen X offset in pixels
 *   ofy (int): screen Y offset in pixels
 *
 * Return value:
 *   None (void)
 */

void func_800B47A8(int ofx, int ofy)
{
    unsigned int gte_ofx = (unsigned int)ofx << 16;
    unsigned int gte_ofy = (unsigned int)ofy << 16;

    /* GTE ctc2 $24 = OFX (screen offset X), ctc2 $25 = OFY (screen offset Y) */
    __asm__ volatile (
        "ctc2 %0, $24\n\t"
        "ctc2 %1, $25"
        :
        : "r"(gte_ofx), "r"(gte_ofy)
    );
}
