/*
 * func_800B5E48 - Build a 3D GTE axis rotation matrix from sin/cos and axis char
 *
 * Purpose:
 *   Copies the 32-byte identity matrix D_800FB5B0 into `mat`, then switches on
 *   `(signed char)(axis - 'X')` (indices 0..0x22 via external jump table
 *   jtbl_800150CC) to populate the 2x2 rotation submatrix for X, Y, or Z axis
 *   using `sin_val` (`a1`) and `cos_val` (`a2`).
 *
 *   Note: Lines 53-55 at the end of the `.s` file contain an adjacent
 *   3-instruction setter (`D_800EF8A8 = arg0;`) sharing the `lui` delay slot
 *   of `.L800B5F08`'s `jr $ra`.
 *
 * Arguments:
 *   mat (unsigned char *): destination 32-byte GTE MATRIX struct (9 s16 + t[3])
 *   sin_val (signed short): sine component (4.12 fixed point)
 *   cos_val (signed short): cosine component (4.12 fixed point)
 *   axis (signed char):     rotation axis character (expected 'X'/'x', 'Y'/'y', 'Z'/'z')
 *
 * Return value:
 *   None (void)
 *
 * Globals read:
 *   D_800FB5B0 (u32[8]): 32-byte identity GTE MATRIX
 *   jtbl_800150CC (void *[0x23]): jump table indexed by `(axis - 0x58)`
 */

#define S(p, o) (*(signed short *)((p) + (o)))
#define W(p, o) (*(unsigned int *)((p) + (o)))

extern unsigned int D_800FB5B0[8];
extern unsigned char *D_800EF8A8;

void func_800B5E48(unsigned char *mat, signed short sin_val, signed short cos_val, signed char axis)
{
    signed char idx;
    signed short neg_sin;

    /* Copy 32-byte identity matrix D_800FB5B0 -> mat */
    W(mat, 0x00) = D_800FB5B0[0];
    W(mat, 0x04) = D_800FB5B0[1];
    W(mat, 0x08) = D_800FB5B0[2];
    W(mat, 0x0C) = D_800FB5B0[3];
    W(mat, 0x10) = D_800FB5B0[4];
    W(mat, 0x14) = D_800FB5B0[5];
    W(mat, 0x18) = D_800FB5B0[6];
    W(mat, 0x1C) = D_800FB5B0[7];

    idx = (signed char)(axis - 0x58); /* 'X' = 0x58 */
    if ((unsigned int)(unsigned char)idx >= 0x23u) {
        return;
    }

    /*
     * UNCLEAR: exact jtbl_800150CC mapping for indices 0..0x22 ('X'..'z') is
     * external data not in this .s file; the four visible jump targets are:
     *   - .L800B5EC4: X-axis rotation
     *   - .L800B5EDC: Y-axis rotation
     *   - .L800B5EF4: Z-axis rotation
     *   - .L800B5F08: default (no-op / identity)
     */
    neg_sin = (signed short)(-sin_val);
    switch (axis) {
    case 'X':
    case 'x':
        /* .L800B5EC4: X-axis rotation (m[1][1]=cos, m[2][2]=cos, m[1][2]=-sin, m[2][1]=sin) */
        S(mat, 0x08) = cos_val;
        S(mat, 0x10) = cos_val;
        S(mat, 0x0A) = neg_sin;
        S(mat, 0x0E) = sin_val;
        break;

    case 'Y':
    case 'y':
        /* .L800B5EDC: Y-axis rotation (m[0][0]=cos, m[2][2]=cos, m[0][2]=sin, m[2][0]=-sin) */
        S(mat, 0x00) = cos_val;
        S(mat, 0x10) = cos_val;
        S(mat, 0x04) = sin_val;
        S(mat, 0x0C) = neg_sin;
        break;

    case 'Z':
    case 'z':
        /* .L800B5EF4: Z-axis rotation (m[0][0]=cos, m[1][1]=cos, m[0][1]=-sin, m[1][0]=sin) */
        S(mat, 0x00) = cos_val;
        S(mat, 0x08) = cos_val;
        S(mat, 0x02) = neg_sin;
        S(mat, 0x06) = sin_val;
        break;

    default:
        /* .L800B5F08 */
        break;
    }
}

/*
 * UNCLEAR: lines 53-55 of func_800B5E48.s (`lui $at, %hi(D_800EF8A8); jr $ra; sw $a0, %lo(D_800EF8A8)($at)`)
 * appear to be an unlabeled 3-instruction setter sharing the `lui` delay slot of .L800B5F08's `jr $ra`.
 */
void func_800B5F10(unsigned char *pkt_ptr)
{
    D_800EF8A8 = pkt_ptr;
}
