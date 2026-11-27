/*
 * func_800B5924
 *
 * Purpose:
 *   Copies the 32-byte (8-word) GTE MATRIX structure at D_800F1078 into the
 *   destination buffer `dst`.
 *
 * Arguments:
 *   dst (unsigned char *): destination 32-byte MATRIX buffer
 *
 * Return value:
 *   None (void)
 *
 * Global fields read:
 *   D_800F1078 (unsigned char[32]): global 32-byte GTE MATRIX
 */

#define W(p, o) (*(unsigned int *)((p) + (o)))

extern unsigned char D_800F1078[];

void func_800B5924(unsigned char *dst)
{
    W(dst, 0x00) = W(D_800F1078, 0x00);
    W(dst, 0x04) = W(D_800F1078, 0x04);
    W(dst, 0x08) = W(D_800F1078, 0x08);
    W(dst, 0x0C) = W(D_800F1078, 0x0C);
    W(dst, 0x10) = W(D_800F1078, 0x10);
    W(dst, 0x14) = W(D_800F1078, 0x14);
    W(dst, 0x18) = W(D_800F1078, 0x18);
    W(dst, 0x1C) = W(D_800F1078, 0x1C);
}
