/*
 * func_800B4B04
 *
 * Purpose:
 *   Composes two 3D transformation matrices (equivalent to PsyQ CompMatrixLV):
 *   computes m2->t = m0->m * m1->t + m0->t via func_800B4E48 (ApplyMatrixLV),
 *   and m2->m = m0->m * m1->m via func_800B4D38 (MulMatrix0).
 *
 * Arguments:
 *   m0 (const unsigned char *) - left MATRIX operand (3x3 shorts at +0x00, s32 t[3] at +0x14)
 *   m1 (const unsigned char *) - right MATRIX operand (3x3 shorts at +0x00, s32 t[3] at +0x14)
 *   m2 (unsigned char *)       - destination MATRIX (3x3 shorts at +0x00, s32 t[3] at +0x14)
 *
 * Return value:
 *   None (void)
 */

#define SW(p, o) (*(signed int *)((p) + (o)))

extern signed int *func_800B4E48(const void *mat, const signed int *in, signed int *out);
extern short *func_800B4D38(const short *m0, const short *m1, short *m2);

void func_800B4B04(const unsigned char *m0, const unsigned char *m1, unsigned char *m2)
{
    func_800B4E48(m0, (const signed int *)(m1 + 0x14), (signed int *)(m2 + 0x14));
    func_800B4D38((const short *)m0, (const short *)m1, (short *)m2);

    SW(m2, 0x14) += SW(m0, 0x14);
    SW(m2, 0x18) += SW(m0, 0x18);
    SW(m2, 0x1C) += SW(m0, 0x1C);
}
