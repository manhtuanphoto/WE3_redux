/*
 * func_8005B36C
 *
 * Purpose:
 *   Computes the 8-bit fixed-point angle (0..255 = 0..360 deg) from (x1, z1) to
 *   (x2, z2) using two 33-entry arctan lookup tables (D_80011FA8 and D_80011FCC)
 *   copied onto the stack. Scales |dx| and |dz| by powers of 2 until at least
 *   one reaches >= 0x21 (33), indexes by `(minor << 5) / major`, and adjusts
 *   for quadrant signs.
 *
 * Arguments:
 *   x1, z1 (int) - origin coordinates (treated as signed 16-bit)
 *   x2, z2 (int) - target coordinates (treated as signed 16-bit)
 *
 * Return value:
 *   unsigned int - 8-bit angle (0..255).
 *
 * Global tables:
 *   D_80011FA8 (u8[33]) - arctan LUT when |dz| < |dx|
 *   D_80011FCC (u8[33]) - arctan LUT when |dx| <= |dz|
 */

extern unsigned char D_80011FA8[33];
extern unsigned char D_80011FCC[33];

unsigned int func_8005B36C(int x1, int z1, int x2, int z2)
{
    unsigned char lut_x[33];
    unsigned char lut_z[33];
    int dx;
    int dz;
    int neg_x;
    int neg_z;
    int ang;
    int i;

    for (i = 0; i < 33; i++) {
        lut_x[i] = D_80011FA8[i];
    }
    for (i = 0; i < 33; i++) {
        lut_z[i] = D_80011FCC[i];
    }

    dx = (signed short)x2 - (signed short)x1;
    dz = (signed short)z2 - (signed short)z1;

    neg_x = 0;
    neg_z = 0;
    if (dx < 0) {
        neg_x = 1;
        dx = -dx;
    }
    if (dz < 0) {
        neg_z = 1;
        dz = -dz;
    }

    /* Note: in assembly, whenever dx == 0, delay slot sets dx = 1 (and if dz == 0 too, dz = 2) */
    if (dx == 0) {
        dx = 1;
        if (dz == 0) {
            dz = 2;
        }
    }

    while (dx < 0x21 && dz < 0x21) {
        dx <<= 1;
        dz <<= 1;
    }

    if (dz < dx) {
        ang = lut_x[(dz << 5) / dx];
    } else {
        ang = lut_z[(dx << 5) / dz];
    }

    if (neg_x != 0) {
        int tmp = 0x180 - ang;
        ang = tmp - ((tmp / 256) * 256);
    }
    if (neg_z != 0) {
        ang = -ang;
    }

    return (unsigned int)(ang & 0xFF);
}
