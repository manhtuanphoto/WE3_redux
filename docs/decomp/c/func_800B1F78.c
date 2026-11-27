/*
 * func_800B1F78
 *
 * Purpose:
 *   Computes a 3x3 fixed-point (4.12) rotation matrix from Euler angles (rx, ry, rz)
 *   using the 4096-entry packed cos/sin lookup table D_800CED64 and GTE GPF
 *   (General Purpose Interpolation, sf=1) operations. Matches PsyQ RotMatrix().
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

static int abs_s16(int v)
{
    int sign = v >> 31;
    return (v + sign) ^ sign;
}

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

short *func_800B1F78(const short *r, short *m)
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

    /* GPF 1: IR0 = cx, IR1 = sy, IR2 = sz, IR3 = cz */
    int cx_sy = (cx * sy) >> 12;
    int cx_sz = (cx * sz) >> 12;
    int cx_cz = (cx * cz) >> 12;

    /* m[2][2] = (cy * cx) >> 12 */
    m[8] = (short)((cy * cx) >> 12);

    /* GPF 1: IR0 = sx, IR1 = sy, IR2 = sz, IR3 = cz */
    int sx_sy = (sx * sy) >> 12;
    int sx_sz = (sx * sz) >> 12;
    int sx_cz = (sx * cz) >> 12;

    /* GPF 1: IR0 = cz, IR1 = cy, IR2 = sx_sy, IR3 = cx_sy */
    int cz_cy = (cz * cy) >> 12;
    int cz_sx_sy = (cz * sx_sy) >> 12;
    int cz_cx_sy = (cz * cx_sy) >> 12;

    /* m[2][0] = -sy, m[2][1] = (cy * sx) >> 12 */
    m[6] = (short)(-sy);
    m[7] = (short)((cy * sx) >> 12);

    /* GPF 1: IR0 = sz, IR1 = cy, IR2 = sx_sy, IR3 = cx_sy */
    int sz_cy = (sz * cy) >> 12;
    int sz_sx_sy = (sz * sx_sy) >> 12;
    int sz_cx_sy = (sz * cx_sy) >> 12;

    m[0] = (short)cz_cy;
    m[1] = (short)(cz_sx_sy - cx_sz);
    m[2] = (short)(cz_cx_sy + sx_sz);
    m[3] = (short)sz_cy;
    m[4] = (short)(sz_sx_sy + cx_cz);
    m[5] = (short)(sz_cx_sy - sx_cz);

    (void)abs_s16;
    return m;
}
