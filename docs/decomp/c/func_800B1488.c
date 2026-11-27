/*
 * func_800B1488
 *
 * Purpose:
 *   Computes a 3x3 fixed-point (Q12) rotation matrix from 3 Euler angles
 *   using the GTE sin/cos lookup table D_800CED64 (packed as (cos << 16) | sin).
 *   Matrix rows/entries correspond to Euler composition (sy at m[2], -cy*sx at m[5],
 *   cy*cx at m[8], cz*cy at m[0], -sz*cy at m[1], etc.).
 *
 * Arguments:
 *   angles - pointer to 3 s16 Euler angles (rx, ry, rz), 4096 = 2*pi
 *   mat    - pointer to output 3x3 s16 matrix (9 elements, 18 bytes)
 *
 * Return value:
 *   Returns `mat`.
 *
 * Global fields read:
 *   D_800CED64 (u32[4096] packed (cos << 16) | (u16)sin)
 */

extern unsigned int D_800CED64[];

static void lookup_sincos(short angle, int *out_sin, int *out_cos)
{
    unsigned int entry;
    int s;

    if (angle < 0) {
        int neg = (-(int)angle) & 0xFFF;
        entry = D_800CED64[neg];
        s = -(int)(short)(entry & 0xFFFF);
    } else {
        entry = D_800CED64[(int)angle & 0xFFF];
        s = (int)(short)(entry & 0xFFFF);
    }
    *out_sin = s;
    *out_cos = (int)entry >> 16;
}

short *func_800B1488(const short *angles, short *mat)
{
    int sx, cx, sy, cy, neg_sy, sz, cz;
    int t;

    lookup_sincos(angles[0], &sx, &cx);
    lookup_sincos(angles[1], &sy, &cy);
    neg_sy = -sy;

    mat[2] = (short)sy;
    mat[5] = (short)(-((int)((unsigned int)cy * (unsigned int)sx)) >> 12);
    mat[8] = (short)((int)((unsigned int)cy * (unsigned int)cx) >> 12);

    lookup_sincos(angles[2], &sz, &cz);

    mat[0] = (short)((int)((unsigned int)cz * (unsigned int)cy) >> 12);
    mat[1] = (short)(-((int)((unsigned int)sz * (unsigned int)cy)) >> 12);

    t = (int)((unsigned int)cz * (unsigned int)neg_sy) >> 12;
    mat[3] = (short)(((int)((unsigned int)sz * (unsigned int)cx) >> 12) -
                     ((int)((unsigned int)t * (unsigned int)sx) >> 12));
    mat[6] = (short)(((int)((unsigned int)sz * (unsigned int)sx) >> 12) +
                     ((int)((unsigned int)t * (unsigned int)cx) >> 12));

    t = (int)((unsigned int)sz * (unsigned int)neg_sy) >> 12;
    mat[4] = (short)(((int)((unsigned int)cz * (unsigned int)cx) >> 12) +
                     ((int)((unsigned int)t * (unsigned int)sx) >> 12));
    mat[7] = (short)(((int)((unsigned int)cz * (unsigned int)sx) >> 12) -
                     ((int)((unsigned int)t * (unsigned int)cx) >> 12));

    return mat;
}
