/*
 * func_800B1348
 *
 * Purpose:
 *   PsyQ libgte ScaleMatrix equivalent. Scales the columns of a 3x3 signed 16-bit
 *   matrix `m` (stored as packed 32-bit words, row-major) by a 3-element 32-bit
 *   fixed-point (4.12) vector `v` (`>> 12`), writing the result back in place.
 *
 * Arguments:
 *   m (signed short (*)[3] / MATRIX *): pointer to 3x3 matrix (0x14-byte struct, 9 x s16)
 *   v (const signed int *): pointer to 3-element scale vector (vx, vy, vz)
 *
 * Return value:
 *   Returns `m`.
 */

signed short *func_800B1348(signed short *m, const signed int *v)
{
    signed int vx = v[0];
    signed int vy = v[1];
    signed int vz = v[2];
    unsigned int w;
    signed short lo;
    signed short hi;

    /* m[0][0], m[0][1] */
    w = *(unsigned int *)(m + 0);
    lo = (signed short)(((signed short)(w & 0xFFFF) * vx) >> 12);
    hi = (signed short)(((signed short)((signed int)w >> 16) * vy) >> 12);
    *(unsigned int *)(m + 0) = ((unsigned int)(unsigned short)hi << 16) | (unsigned short)lo;

    /* m[0][2], m[1][0] */
    w = *(unsigned int *)(m + 2);
    lo = (signed short)(((signed short)(w & 0xFFFF) * vz) >> 12);
    hi = (signed short)(((signed short)((signed int)w >> 16) * vx) >> 12);
    *(unsigned int *)(m + 2) = ((unsigned int)(unsigned short)hi << 16) | (unsigned short)lo;

    /* m[1][1], m[1][2] */
    w = *(unsigned int *)(m + 4);
    lo = (signed short)(((signed short)(w & 0xFFFF) * vy) >> 12);
    hi = (signed short)(((signed short)((signed int)w >> 16) * vz) >> 12);
    *(unsigned int *)(m + 4) = ((unsigned int)(unsigned short)hi << 16) | (unsigned short)lo;

    /* m[2][0], m[2][1] */
    w = *(unsigned int *)(m + 6);
    lo = (signed short)(((signed short)(w & 0xFFFF) * vx) >> 12);
    hi = (signed short)(((signed short)((signed int)w >> 16) * vy) >> 12);
    *(unsigned int *)(m + 6) = ((unsigned int)(unsigned short)hi << 16) | (unsigned short)lo;

    /* m[2][2] (written as full 32-bit word at +0x10) */
    w = *(unsigned int *)(m + 8);
    *(signed int *)(m + 8) = ((signed short)(w & 0xFFFF) * vz) >> 12;

    return m;
}
