/*
 * func_800B4C14
 *
 * Purpose:
 *   Composes two 3D MATRIX transforms in-place into the first operand: m0 = m0 * m1.
 *   Transforms m1's translation vector (+0x14..+0x1C) by m0's 3x3 rotation matrix
 *   into a temporary stack vector via func_800B4E48, multiplies m0's 3x3 rotation
 *   matrix by m1 in-place (m0 = m0 * m1) via func_800B50F8, and adds the
 *   transformed translation to m0's translation vector (+0x14..+0x1C).
 *
 * Arguments:
 *   m0 (unsigned char *): left MATRIX operand and destination (32-byte MATRIX)
 *   m1 (unsigned char *): right MATRIX operand (32-byte MATRIX)
 *
 * Return value:
 *   None (void)
 */

#define SW(p, o) (*(signed int *)((unsigned char *)(p) + (o)))

extern signed int *func_800B4E48(const void *mat, const signed int *in, signed int *out);
extern short *func_800B50F8(short *m0, const short *m1);

void func_800B4C14(unsigned char *m0, unsigned char *m1)
{
    signed int trans[3];

    func_800B4E48(m0, (const signed int *)(m1 + 0x14), trans);
    func_800B50F8((short *)m0, (const short *)m1);

    SW(m0, 0x14) = trans[0] + SW(m0, 0x14);
    SW(m0, 0x18) = trans[1] + SW(m0, 0x18);
    SW(m0, 0x1C) = trans[2] + SW(m0, 0x1C);
}
