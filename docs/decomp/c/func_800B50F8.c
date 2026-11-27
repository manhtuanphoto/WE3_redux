/*
 * func_800B50F8
 *
 * Purpose:
 *   PS1 GTE 3x3 fixed-point (4.12) matrix multiply: m0 = m0 * m1 (PsyQ MulMatrix).
 *   Loads 3x3 rotation matrix m0 into GTE control registers RT ($0..$4), multiplies
 *   each column of m1 via `mvmva 1, 0, 0, 3, 0` (shift 12, lm=0), and stores the
 *   resulting 3x3 matrix back into m0.
 *
 * Arguments:
 *   m0 (MATRIX * / short *): left 3x3 matrix operand and destination (+0x00..+0x13)
 *   m1 (MATRIX * / short *): right 3x3 matrix operand (+0x00..+0x11)
 *
 * Return value:
 *   m0
 *
 * Notes:
 *   GTE `mvmva 1, 0, 0, 3, 0` clamps IR1..IR3 to signed 16-bit [-0x8000, 0x7FFF].
 *   The final `swc2 $11, 0x10($a0)` writes a full 32-bit sign-extended IR3 word at
 *   offset +0x10 (updating m0[8] at +0x10 and 2 bytes of padding at +0x12).
 */

static short clamp_ir16(int val)
{
    if (val < -0x8000) {
        return -0x8000;
    }
    if (val > 0x7FFF) {
        return 0x7FFF;
    }
    return (short)val;
}

short *func_800B50F8(short *m0, const short *m1)
{
    short col0[3];
    short col1[3];
    short col2[3];
    int r;

    /* Column 0 of m1: (m1[0], m1[3], m1[6]) */
    for (r = 0; r < 3; r++) {
        int acc = (int)m0[r * 3 + 0] * (int)m1[0]
                + (int)m0[r * 3 + 1] * (int)m1[3]
                + (int)m0[r * 3 + 2] * (int)m1[6];
        col0[r] = clamp_ir16(acc >> 12);
    }

    /* Column 1 of m1: (m1[1], m1[4], m1[7]) */
    for (r = 0; r < 3; r++) {
        int acc = (int)m0[r * 3 + 0] * (int)m1[1]
                + (int)m0[r * 3 + 1] * (int)m1[4]
                + (int)m0[r * 3 + 2] * (int)m1[7];
        col1[r] = clamp_ir16(acc >> 12);
    }

    /* Column 2 of m1: (m1[2], m1[5], m1[8]) */
    for (r = 0; r < 3; r++) {
        int acc = (int)m0[r * 3 + 0] * (int)m1[2]
                + (int)m0[r * 3 + 1] * (int)m1[5]
                + (int)m0[r * 3 + 2] * (int)m1[8];
        col2[r] = clamp_ir16(acc >> 12);
    }

    m0[0] = col0[0]; m0[1] = col1[0]; m0[2] = col2[0];
    m0[3] = col0[1]; m0[4] = col1[1]; m0[5] = col2[1];
    m0[6] = col0[2]; m0[7] = col1[2];
    /* Mirrors `swc2 $11, 0x10($a0)` 32-bit sign-extended write at +0x10 */
    *(int *)((unsigned char *)m0 + 0x10) = (int)col2[2];

    return m0;
}
