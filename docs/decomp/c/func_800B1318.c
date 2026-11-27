/*
 * func_800B1318
 *
 * Purpose:
 *   Sets the translation vector of a PsyQ MATRIX struct (TransMatrix pattern).
 *   Copies three 32-bit words (vx, vy, vz) from `trans` (+0x00, +0x04, +0x08)
 *   into `mat` at offsets +0x14, +0x18, +0x1C (`MATRIX.t[0..2]`) and returns `mat`.
 *
 * Arguments:
 *   mat   (unsigned char *): pointer to PsyQ MATRIX struct (32 bytes; t[3] at +0x14)
 *   trans (const unsigned char *): pointer to PsyQ VECTOR struct (12 bytes: vx, vy, vz)
 *
 * Return value:
 *   unsigned char *: returns `mat`
 */

#define W(p, o) (*(unsigned int *)((p) + (o)))
#define CW(p, o) (*(const unsigned int *)((p) + (o)))

unsigned char *func_800B1318(unsigned char *mat, const unsigned char *trans)
{
    unsigned int tx = CW(trans, 0x0);
    unsigned int ty = CW(trans, 0x4);
    unsigned int tz = CW(trans, 0x8);

    W(mat, 0x14) = tx;
    W(mat, 0x18) = ty;
    W(mat, 0x1C) = tz;

    return mat;
}
