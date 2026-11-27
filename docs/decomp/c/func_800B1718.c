/*
 * func_800B1718
 *
 * Purpose:
 *   Computes a 3x3 fixed-point (Q12) rotation matrix from 3 Euler angles
 *   (YXZ-order variant: -sx at m[5], sy*cx at m[2], cy*cx at m[8], sz*cx at m[3],
 *   cz*cx at m[4]) using the GTE sin/cos lookup table D_800CED64.
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

short *func_800B1718(const short *angles, short *mat)
{
    int sx, cx, neg_sx, sy, cy, sz, cz;
    int t;

    lookup_sincos(angles[0], &sx, &cx);
    neg_sx = -sx;
    lookup_sincos(angles[1], &sy, &cy);

    mat[5] = (short)neg_sx;
    mat[2] = (short)((int)((unsigned int)sy * (unsigned int)cx) >> 12);
    mat[8] = (short)((int)((unsigned int)cy * (unsigned int)cx) >> 12);

    lookup_sincos(angles[2], &sz, &cz);

    mat[3] = (short)((int)((unsigned int)sz * (unsigned int)cx) >> 12);
    mat[4] = (short)((int)((unsigned int)cz * (unsigned int)cx) >> 12);

    t = (int)((unsigned int)sy * (unsigned int)sx) >> 12;
    mat[0] = (short)(((int)((unsigned int)cy * (unsigned int)cz) >> 12) +
                     ((int)((unsigned int)t * (unsigned int)sz) >> 12));
    mat[1] = (short)((-((int)((unsigned int)cy * (unsigned int)sz) >> 12)) +
                     ((int)((unsigned int)t * (unsigned int)cz) >> 12));

    t = (int)((unsigned int)cy * (unsigned int)sx) >> 12;
    mat[7] = (short)(((int)((unsigned int)sy * (unsigned int)sz) >> 12) +
                     ((int)((unsigned int)t * (unsigned int)cz) >> 12));
    mat[6] = (short)((-((int)((unsigned int)sy * (unsigned int)cz) >> 12)) +
                     ((int)((unsigned int)t * (unsigned int)sz) >> 12));

    return mat;
}
