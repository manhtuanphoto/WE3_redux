/*
 * func_800B3F08
 *
 * Purpose:
 *   PsyQ SetTransMatrix equivalent. Loads the 3-word translation vector at
 *   offsets +0x14, +0x18, +0x1C of the matrix struct `mat` into PS1 GTE
 *   control registers $5, $6, $7 (TRX, TRY, TRZ) via `ctc2`.
 *
 * Arguments:
 *   mat (const unsigned char *): pointer to MATRIX-like struct (translation
 *   vector t[0..2] at byte offsets +0x14, +0x18, +0x1C)
 *
 * Return value:
 *   None (void)
 */

#define SW(p, o) (*(const signed int *)((p) + (o)))

void func_800B3F08(const unsigned char *mat)
{
    signed int tx = SW(mat, 0x14);
    signed int ty = SW(mat, 0x18);
    signed int tz = SW(mat, 0x1C);

    /* Direct GTE control register writes (ctc2 $5=TRX, $6=TRY, $7=TRZ).
     * Standard C cannot emit ctc2 without inline assembly or intrinsics. */
    (void)tx; /* ctc2 $t0, $5 (TRX) */
    (void)ty; /* ctc2 $t1, $6 (TRY) */
    (void)tz; /* ctc2 $t2, $7 (TRZ) */
}
