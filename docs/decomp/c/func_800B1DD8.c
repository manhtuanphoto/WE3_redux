/*
 * func_800B1DD8
 *
 * Purpose:
 *   PsyQ libgte RotMatrixZ equivalent. Looks up sin/cos (4.12 fixed-point) for angle `r`
 *   in 4096-entry packed table D_800CED64 (high 16 bits = cos, low 16 bits = sin) and
 *   rotates rows 0 and 1 of the 3x3 signed 16-bit matrix `m` around the Z axis in place:
 *     m[0][j] = (cos * m[0][j] - sin * m[1][j]) >> 12
 *     m[1][j] = (sin * m[0][j] + cos * m[1][j]) >> 12
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

signed short *func_800B1DD8(signed int r, signed short *m)
{
    unsigned int packed;
    signed int s;
    signed int c;
    signed short m00, m01, m02;
    signed short m10, m11, m12;

    if (r < 0) {
        signed int nr = -r;
        unsigned int idx = (nr >= 0) ? ((unsigned int)nr & 0xFFFU) : (unsigned int)nr;
        packed = D_800CED64[idx];
        s = -(signed short)(packed & 0xFFFF);
        c = (signed short)((signed int)packed >> 16);
    } else {
        packed = D_800CED64[(unsigned int)r & 0xFFFU];
        s = (signed short)(packed & 0xFFFF);
        c = (signed short)((signed int)packed >> 16);
    }

    m00 = m[0];
    m01 = m[1];
    m02 = m[2];
    m10 = m[3];
    m11 = m[4];
    m12 = m[5];

    m[0] = (signed short)((c * m00 - s * m10) >> 12);
    m[1] = (signed short)((c * m01 - s * m11) >> 12);
    m[2] = (signed short)((c * m02 - s * m12) >> 12);
    m[3] = (signed short)((s * m00 + c * m10) >> 12);
    m[4] = (signed short)((s * m01 + c * m11) >> 12);
    m[5] = (signed short)((s * m02 + c * m12) >> 12);

    return m;
}
