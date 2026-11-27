/*
 * func_800B19A8
 *
 * Purpose:
 *   Computes a 3x3 fixed-point (Q12) rotation matrix from 3 Euler angles
 *   (XYZ-order variant: -sy at m[6], sx*cy at m[7], cx*cy at m[8], cy*cz at m[0],
 *   sz*cy at m[3]) using the GTE sin/cos lookup table D_800CED64.
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

short *func_800B19A8(const short *angles, short *mat)
{
    int sx, cx, sy, cy, neg_sy, sz, cz;
    int t;

    lookup_sincos(angles[0], &sx, &cx);
    lookup_sincos(angles[1], &sy, &cy);
    neg_sy = -sy;

    mat[6] = (short)neg_sy;
    mat[7] = (short)((int)((unsigned int)sx * (unsigned int)cy) >> 12);
    mat[8] = (short)((int)((unsigned int)cx * (unsigned int)cy) >> 12);

    lookup_sincos(angles[2], &sz, &cz);

    mat[0] = (short)((int)((unsigned int)cy * (unsigned int)cz) >> 12);
    mat[3] = (short)((int)((unsigned int)sz * (unsigned int)cy) >> 12);

    t = (int)((unsigned int)sx * (unsigned int)sy) >> 12;
    mat[1] = (short)(((int)((unsigned int)t * (unsigned int)cz) >> 12) -
                     ((int)((unsigned int)sz * (unsigned int)cx) >> 12));
    mat[4] = (short)(((int)((unsigned int)cx * (unsigned int)cz) >> 12) +
                     ((int)((unsigned int)t * (unsigned int)sz) >> 12));

    t = (int)((unsigned int)sy * (unsigned int)cx) >> 12;
    mat[2] = (short)(((int)((unsigned int)t * (unsigned int)cz) >> 12) +
                     ((int)((unsigned int)sx * (unsigned int)sz) >> 12));
    mat[5] = (short)(((int)((unsigned int)t * (unsigned int)sz) >> 12) -
                     ((int)((unsigned int)sx * (unsigned int)cz) >> 12));

    return mat;
}
