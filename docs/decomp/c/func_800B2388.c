/*
 * func_800B2388
 *
 * Purpose:
 *   Computes a 3x3 fixed-point (4.12) rotation matrix in YXZ Euler order from
 *   angles (rx, ry, rz) using the 4096-entry packed cos/sin lookup table D_800CED64
 *   and GTE GPF (sf=1) operations. Matches PsyQ RotMatrixYXZ().
 *
 * Arguments:
 *   r (short *): pointer to 3 signed 16-bit angles (rx at +0x0, ry at +0x2, rz at +0x4; 4096 = 2*pi)
 *   m (short *): pointer to output 3x3 row-major matrix (+0x00..+0x11, 16-bit elements)
 *
 * Return value:
 *   Returns m (pointer passed in a1).
 *
 * Global fields read:
 *   D_800CED64 (u32[4096]): packed (cos << 16) | (sin & 0xFFFF) table indexed by (abs(angle) & 0xFFF)
 *
 * Global fields written:
 *   None
 *
 * GTE registers used:
 *   $8 (IR0), $9 (IR1), $10 (IR2), $11 (IR3) via GPF sf=1 (12-bit fixed-point multiply)
 */

extern unsigned int D_800CED64[];

static unsigned int lookup_cossin(int angle)
{
    int sign = angle >> 31;
    int abs_ang = (angle + sign) ^ sign;
    unsigned int packed = D_800CED64[abs_ang & 0xFFF];
    int cos_part = (int)packed >> 16;
    int sin_part = (int)(packed << 16);
    sin_part = (sin_part + sign) ^ sign;
    return ((unsigned int)cos_part << 16) | ((unsigned int)sin_part >> 16);
}

short *func_800B2388(const short *r, short *m)
{
    int rx = r[0];
    int ry = r[1];
    int rz = r[2];

    unsigned int cs_x = lookup_cossin(rx);
    unsigned int cs_y = lookup_cossin(ry);
    unsigned int cs_z = lookup_cossin(rz);

    int sx = (short)(cs_x & 0xFFFF);
    int cx = (int)cs_x >> 16;
    int sy = (short)(cs_y & 0xFFFF);
    int cy = (int)cs_y >> 16;
    int sz = (short)(cs_z & 0xFFFF);
    int cz = (int)cs_z >> 16;

    /* GPF 1: IR0 = cy, IR1 = sx, IR2 = sz, IR3 = cz */
    int cy_sx = (cy * sx) >> 12;
    int cy_sz = (cy * sz) >> 12;
    int cy_cz = (cy * cz) >> 12;

    /* GPF 1: IR0 = sy, IR1 = sx, IR2 = sz, IR3 = cz */
    int sy_sx = (sy * sx) >> 12;
    int sy_sz = (sy * sz) >> 12;
    int sy_cz = (sy * cz) >> 12;

    /* m[2][2] = (cx * cy) >> 12 */
    m[8] = (short)((cx * cy) >> 12);

    /* GPF 1: IR0 = cz, IR1 = cx, IR2 = sy_sx, IR3 = cy_sx */
    int cz_cx = (cz * cx) >> 12;
    int cz_sy_sx = (cz * sy_sx) >> 12;
    int cz_cy_sx = (cz * cy_sx) >> 12;

    /* GPF 1: IR0 = sz, IR1 = cx, IR2 = sy_sx, IR3 = cy_sx */
    int sz_cx = (sz * cx) >> 12;
    int sz_sy_sx = (sz * sy_sx) >> 12;
    int sz_cy_sx = (sz * cy_sx) >> 12;

    m[4] = (short)cz_cx;
    m[5] = (short)(-sx);
    m[0] = (short)(cy_cz + sz_sy_sx);
    m[1] = (short)(cz_sy_sx - cy_sz);
    m[2] = (short)((cx * sy) >> 12);
    m[3] = (short)sz_cx;
    m[6] = (short)(sz_cy_sx - sy_cz);
    m[7] = (short)(cz_cy_sx + sy_sz);

    return m;
}
