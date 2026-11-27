/*
 * func_800B5978
 *
 * Purpose:
 *   Loads a 3x3 matrix (5 packed 32-bit words / 20 bytes) into PS1 GTE (COP2)
 *   control registers $16-$20 (Color Matrix: LR1LR2, LR3LG1, LG2LG3, LB1LB2, LB3).
 *   Matches PsyQ libgte SetColorMatrix().
 *
 * Arguments:
 *   mat (const unsigned int *): Pointer to 5x 32-bit packed matrix words
 *
 * Return value:
 *   None (void)
 */

static inline void gte_ctc2_color_matrix(unsigned int r16, unsigned int r17,
                                         unsigned int r18, unsigned int r19,
                                         unsigned int r20)
{
    /* Hardware GTE (COP2) control registers $16..$20 */
    __asm__ volatile (
        "ctc2 %0, $16\n"
        "ctc2 %1, $17\n"
        "ctc2 %2, $18\n"
        "ctc2 %3, $19\n"
        "ctc2 %4, $20\n"
        :
        : "r"(r16), "r"(r17), "r"(r18), "r"(r19), "r"(r20)
    );
}

void func_800B5978(const unsigned int *mat)
{
    gte_ctc2_color_matrix(mat[0], mat[1], mat[2], mat[3], mat[4]);
}
