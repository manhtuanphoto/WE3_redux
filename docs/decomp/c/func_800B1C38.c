/*
 * func_800B1C38
 *
 * Purpose:
 *   PsyQ libgte RotMatrixY equivalent. Looks up sin/cos (4.12 fixed-point) for angle `r`
 *   in 4096-entry packed table D_800CED64 (high 16 bits = cos, low 16 bits = sin) and
 *   rotates rows 0 and 2 of the 3x3 signed 16-bit matrix `m` around the Y axis in place:
 *     m[0][j] = (cos * m[0][j] + sin * m[2][j]) >> 12
 *     m[2][j] = (-sin * m[0][j] + cos * m[2][j]) >> 12
 *
 * Arguments:
 *   r (signed int): rotation angle (4096 = 360 degrees)
 *   m (signed short *): pointer to 3x3 matrix (row-major, 9 x s16)
 *
 * Return value:
 *   Returns `m`.
 *
 * Global fields read:
 *   D_800CED64 (const unsigned int[4096]): packed (cos << 16) | (u16)sin lookup table
 */

extern const unsigned int D_800CED64[4096];

signed short *func_800B1C38(signed int r, signed short *m)
{
    unsigned int packed;
    signed int neg_sin;
    signed int c;
    signed short m00, m01, m02;
    signed short m20, m21, m22;

    if (r < 0) {
        signed int nr = -r;
        unsigned int idx = (nr >= 0) ? ((unsigned int)nr & 0xFFFU) : (unsigned int)nr;
        packed = D_800CED64[idx];
        neg_sin = (signed short)(packed & 0xFFFF);
        c = (signed short)((signed int)packed >> 16);
    } else {
        packed = D_800CED64[(unsigned int)r & 0xFFFU];
        neg_sin = -(signed short)(packed & 0xFFFF);
        c = (signed short)((signed int)packed >> 16);
    }

    m00 = m[0];
    m01 = m[1];
    m02 = m[2];
    m20 = m[6];
    m21 = m[7];
    m22 = m[8];

    m[0] = (signed short)((c * m00 - neg_sin * m20) >> 12);
    m[1] = (signed short)((c * m01 - neg_sin * m21) >> 12);
    m[2] = (signed short)((c * m02 - neg_sin * m22) >> 12);
    m[6] = (signed short)((neg_sin * m00 + c * m20) >> 12);
    m[7] = (signed short)((neg_sin * m01 + c * m21) >> 12);
    m[8] = (signed short)((neg_sin * m02 + c * m22) >> 12);

    return m;
}
