/*
 * func_800B2178
 *
 * Purpose:
 *   Computes a 3x3 fixed-point rotation matrix (Q12, 4096 = 1.0) from 16-bit
 *   Euler angles (rx, ry, rz in 0..4095) using the PS1 GTE `GPF 1` (General
 *   Purpose Interpolation, sf=1 -> >> 12) instruction and the 4096-entry packed
 *   sin/cos table D_800CED64 (`(cos << 16) | (u16)sin` for angles 0..2047/4095).
 *   Equivalent to PS1 libgte `RotMatrix`.
 *   Note: GTE COP2 register mapping used by `gpf 1`:
 *     $8 = IR0 (scalar), $9 = IR1, $10 = IR2, $11 = IR3.
 *
 * Arguments:
 *   rot    (signed short *)  - input SVECTOR {vx, vy, vz} (reads +0, +2, +4)
 *   outMat (unsigned char *) - output PS1 MATRIX (writes 3x3 s16 m[3][3] at +0x00..+0x10)
 *
 * Return value:
 *   None (void) — $v0 holds `outMat` on exit in assembly.
 *
 * Global table:
 *   D_800CED64 (u32[4096]) - packed (cos << 16) | (u16)sin table
 */

#define H(p, o) (*(unsigned short *)((p) + (o)))
#define S(p, o) (*(signed short *)((p) + (o)))
#define W(p, o) (*(unsigned int *)((p) + (o)))

extern unsigned int D_800CED64[];

void func_800B2178(signed short *rot, unsigned char *outMat)
{
    int rx = rot[0];
    int ry = rot[1];
    int rz = rot[2];
    int abs_rx = (rx < 0) ? -rx : rx;
    int abs_ry = (ry < 0) ? -ry : ry;
    int abs_rz = (rz < 0) ? -rz : rz;
    unsigned int pack_x = D_800CED64[abs_rx & 0xFFF];
    unsigned int pack_y = D_800CED64[abs_ry & 0xFFF];
    unsigned int pack_z = D_800CED64[abs_rz & 0xFFF];
    int sx = (signed short)(pack_x & 0xFFFF);
    int cx = (signed short)(pack_x >> 16);
    int sy = (signed short)(pack_y & 0xFFFF);
    int cy = (signed short)(pack_y >> 16);
    int sz = (signed short)(pack_z & 0xFFFF);
    int cz = (signed short)(pack_z >> 16);
    int sy_cx, sz_cx, cz_cx;
    int sy_sx, sz_sx, cz_sx;
    int cy_cz, sy_sx_cz, sy_cx_cz;
    int cy_sz, sy_sx_sz, sy_cx_sz;

    if (rx < 0) {
        sx = -sx;
    }
    if (ry < 0) {
        sy = -sy;
    }
    if (rz < 0) {
        sz = -sz;
    }

    /* GTE GPF 1: (IR1, IR2, IR3) * IR0 >> 12 with IR0 = cx */
    sy_cx = (sy * cx) >> 12;
    sz_cx = (sz * cx) >> 12;
    cz_cx = (cz * cx) >> 12;

    /* GTE GPF 1: (IR1, IR2, IR3) * IR0 >> 12 with IR0 = sx; CPU mult cy * cx >> 12 */
    sy_sx = (sy * sx) >> 12;
    sz_sx = (sz * sx) >> 12;
    cz_sx = (cz * sx) >> 12;
    S(outMat, 0x10) = (signed short)((cy * cx) >> 12); /* m[2][2] */

    /* GTE GPF 1: (cy, sy_sx, sy_cx) * cz >> 12; CPU mult cy * sx >> 12 */
    cy_cz    = (cy * cz) >> 12;
    sy_sx_cz = (sy_sx * cz) >> 12;
    sy_cx_cz = (sy_cx * cz) >> 12;

    /* GTE GPF 1: (cy, sy_sx, sy_cx) * sz >> 12 */
    cy_sz    = (cy * sz) >> 12;
    sy_sx_sz = (sy_sx * sz) >> 12;
    sy_cx_sz = (sy_cx * sz) >> 12;

    W(outMat, 0x00) = (unsigned int)(((-cy_sz) << 16) | (cy_cz & 0xFFFF));          /* m[0][0], m[0][1] */
    W(outMat, 0x04) = (unsigned int)(((sz_cx + sy_sx_cz) << 16) | (sy & 0xFFFF));   /* m[0][2], m[1][0] */
    W(outMat, 0x08) = (unsigned int)(((-((cy * sx) >> 12)) << 16) | ((cz_cx - sy_sx_sz) & 0xFFFF)); /* m[1][1], m[1][2] */
    W(outMat, 0x0C) = (unsigned int)(((cz_sx + sy_cx_sz) << 16) | ((sz_sx - sy_cx_cz) & 0xFFFF));   /* m[2][0], m[2][1] */
}
