/*
 * func_800B3ED8
 *
 * Purpose:
 *   Loads a 3x3 matrix (5 packed 32-bit words / 20 bytes) into PS1 GTE (COP2)
 *   control registers $0-$4 (Rotation Matrix: R11R12, R13R21, R22R23, R31R32, R33).
 *   Matches PsyQ libgte SetRotMatrix().
 *
 * Arguments:
 *   mat (const unsigned int *): Pointer to 5x 32-bit packed matrix words
 *
 * Return value:
 *   None (void)
 */

static inline void gte_ctc2_rot_matrix(unsigned int r0, unsigned int r1,
                                       unsigned int r2, unsigned int r3,
                                       unsigned int r4)
{
    /* Hardware GTE (COP2) control registers $0..$4 */
    __asm__ volatile (
        "ctc2 %0, $0\n"
        "ctc2 %1, $1\n"
        "ctc2 %2, $2\n"
        "ctc2 %3, $3\n"
        "ctc2 %4, $4\n"
        :
        : "r"(r0), "r"(r1), "r"(r2), "r"(r3), "r"(r4)
    );
}

void func_800B3ED8(const unsigned int *mat)
{
    gte_ctc2_rot_matrix(mat[0], mat[1], mat[2], mat[3], mat[4]);
}
