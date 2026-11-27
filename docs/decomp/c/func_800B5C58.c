/*
 * func_800B5C58
 *
 * Purpose:
 *   PsyQ SetFarColor equivalent. Shifts each RGB component left by 4 and
 *   writes them to PS1 GTE Far Color control registers $21, $22, $23
 *   (RFC, GFC, BFC) via `ctc2`.
 *
 * Arguments:
 *   r (int): red component (shifted left by 4 for GTE RFC)
 *   g (int): green component (shifted left by 4 for GTE GFC)
 *   b (int): blue component (shifted left by 4 for GTE BFC)
 *
 * Return value:
 *   None (void)
 */

void func_800B5C58(int r, int g, int b)
{
    int rfc = r << 4;
    int gfc = g << 4;
    int bfc = b << 4;

    /* Direct GTE control register writes (ctc2 $21=RFC, $22=GFC, $23=BFC).
     * Standard C cannot emit ctc2 without inline assembly or intrinsics. */
    (void)rfc; /* ctc2 $a0, $21 (RFC) */
    (void)gfc; /* ctc2 $a1, $22 (GFC) */
    (void)bfc; /* ctc2 $a2, $23 (BFC) */
}
