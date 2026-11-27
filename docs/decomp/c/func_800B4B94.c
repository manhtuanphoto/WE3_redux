/*
 * func_800B4B94
 *
 * Purpose:
 *   Composes two 3D MATRIX transforms (PsyQ CompMatrix-style): m1 = m0 * m1.
 *   Transforms m1's translation vector (+0x14..+0x1C) by m0's 3x3 rotation matrix
 *   into a temporary stack vector via func_800B4E48, multiplies m1's 3x3 rotation
 *   matrix by m0 in-place (m1 = m0 * m1) via func_800B11B8, and adds m0's
 *   translation vector (+0x14..+0x1C) to complete m1's world translation.
 *
 * Arguments:
 *   m0 (unsigned char *): parent MATRIX (3x3 s16 rotation at +0x00..+0x11, s32[3] t at +0x14..+0x1F)
 *   m1 (unsigned char *): local MATRIX operand and destination
 *
 * Return value:
 *   None (void)
 */

#define SW(p, o) (*(signed int *)((unsigned char *)(p) + (o)))

extern signed int *func_800B4E48(const void *mat, const signed int *in, signed int *out);
extern short *func_800B11B8(const short *m0, short *m1);

void func_800B4B94(unsigned char *m0, unsigned char *m1)
{
    signed int trans[3];

    func_800B4E48(m0, (const signed int *)(m1 + 0x14), trans);
    func_800B11B8((const short *)m0, (short *)m1);

    SW(m1, 0x14) = trans[0] + SW(m0, 0x14);
    SW(m1, 0x18) = trans[1] + SW(m0, 0x18);
    SW(m1, 0x1C) = trans[2] + SW(m0, 0x1C);
}
