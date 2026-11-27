/*
 * func_800B5208
 *
 * Purpose:
 *   Loads a 3x3 matrix (5 packed 32-bit words / 20 bytes) into PS1 GTE (COP2)
 *   control registers $8-$12 (Light Matrix: L11L12, L13L21, L22L23, L31L32, L33).
 *   Matches PsyQ libgte SetLightMatrix().
 *
 * Arguments:
 *   mat (const unsigned int *): Pointer to 5x 32-bit packed matrix words
 *
 * Return value:
 *   None (void)
 */

static inline void gte_ctc2_light_matrix(unsigned int r8, unsigned int r9,
                                         unsigned int r10, unsigned int r11,
                                         unsigned int r12)
{
    /* Hardware GTE (COP2) control registers $8..$12 */
    __asm__ volatile (
        "ctc2 %0, $8\n"
        "ctc2 %1, $9\n"
        "ctc2 %2, $10\n"
        "ctc2 %3, $11\n"
        "ctc2 %4, $12\n"
        :
        : "r"(r8), "r"(r9), "r"(r10), "r"(r11), "r"(r12)
    );
}

void func_800B5208(const unsigned int *mat)
{
    gte_ctc2_light_matrix(mat[0], mat[1], mat[2], mat[3], mat[4]);
}
