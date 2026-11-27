/*
 * func_800B6588
 *
 * Purpose:
 *   Applies a Z-axis rotation (roll) by `angle` to the 32-byte transform matrix
 *   `mat`. Divides `angle` by 360 (via signed multiply by 0xB60B60B7), evaluates
 *   cosine via func_800B6718 and sine via func_800B6648, builds a 32-byte Z-axis
 *   rotation matrix on the stack (if `angle != 0`), and multiplies it into
 *   `mat` via func_800B50F8.
 *
 * Arguments:
 *   mat   (u8*) - pointer to 32-byte GTE matrix (3x3 s16 + pad + s32[3] trans)
 *   angle (s32) - signed rotation angle (divided by 360 before trig lookup)
 *
 * Return value:
 *   None (void)
 */

#define S(p, o)  (*(signed short *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))

extern int func_800B6718(int angle);
extern int func_800B6648(int angle);
extern void func_800B50F8(unsigned char *dstMat, unsigned char *srcMat);

void func_800B6588(unsigned char *mat, signed int angle)
{
    unsigned char rotMat[32];
    signed int scaledAngle;
    signed short cosVal;
    signed short sinVal;

    /* Compiler-emitted signed division by 360 (0xB60B60B7 magic multiplier) */
    scaledAngle = angle / 360;
    cosVal = (signed short)func_800B6718(scaledAngle);
    sinVal = (signed short)func_800B6648(scaledAngle);

    if (angle != 0) {
        S(rotMat, 0x00) = cosVal;
        S(rotMat, 0x02) = (signed short)(-sinVal);
        S(rotMat, 0x04) = 0;
        S(rotMat, 0x06) = sinVal;
        S(rotMat, 0x08) = cosVal;
        S(rotMat, 0x0A) = 0;
        S(rotMat, 0x0C) = 0;
        S(rotMat, 0x0E) = 0;
        S(rotMat, 0x10) = 0x1000; /* 1.0 in 4.12 fixed point */
        SW(rotMat, 0x14) = 0;
        SW(rotMat, 0x18) = 0;
        SW(rotMat, 0x1C) = 0;

        func_800B50F8(mat, rotMat);
    }
}
