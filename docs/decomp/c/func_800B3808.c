/*
 * func_800B3808
 *
 * Purpose:
 *   Copies a GPU polygon primitive from `prim` into the active primitive buffer
 *   at D_800EF8A8, offsetting all vertex (x, y) coordinates by (D_800F1070,
 *   D_800F1072). Supports Gouraud colors (flag 0x04 at +0x7), texture/CLUT/tpage
 *   words (flag 0x10 at +0x7), and 4-vertex quads (flag 0x08 at +0x7). Calls
 *   func_800B3578 to link/commit the primitive and updates D_800EF8A8 with the
 *   next free buffer pointer.
 *
 * Arguments:
 *   prim - pointer to source GPU polygon primitive
 *   arg1 - passed through to func_800B3578
 *   arg2 - u16 passed through to func_800B3578
 *
 * Return value:
 *   None (void)
 *
 * Global fields read / written:
 *   D_800EF8A8 (u8 * current GPU primitive buffer pointer; updated on return)
 *   D_800F1070 (u16 screen X offset)
 *   D_800F1072 (u16 screen Y offset)
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))

extern unsigned char *D_800EF8A8;
extern unsigned short D_800F1070;
extern unsigned short D_800F1072;

extern unsigned char *func_800B3578(
    unsigned char *dst,
    unsigned int arg1,
    unsigned short arg2,
    unsigned int word_count
);

void func_800B3808(const unsigned char *prim, unsigned int arg1, unsigned short arg2)
{
    const unsigned char *src = prim;
    unsigned char *dst = D_800EF8A8;
    unsigned char flags = B(src, 0x07);
    unsigned int has_tex = flags & 0x10;
    unsigned int is_quad = flags & 0x08;
    unsigned int is_gouraud = flags & 0x04;
    unsigned int words = 4;

    W(dst, 0x04) = W(src, 0x04);
    H(dst, 0x08) = (unsigned short)(H(src, 0x08) + D_800F1070);
    H(dst, 0x0A) = (unsigned short)(H(src, 0x0A) + D_800F1072);

    if (is_gouraud) {
        W(dst, 0x0C) = W(src, 0x0C);
        src += 4;
        dst += 4;
        words = 5;
    }

    if (has_tex) {
        W(dst, 0x0C) = W(src, 0x0C);
        src += 4;
        dst += 4;
        words++;
    }

    H(dst, 0x0C) = (unsigned short)(H(src, 0x0C) + D_800F1070);
    H(dst, 0x0E) = (unsigned short)(H(src, 0x0E) + D_800F1072);

    if (is_gouraud) {
        W(dst, 0x10) = W(src, 0x10);
        src += 4;
        dst += 4;
        words++;
    }

    if (has_tex) {
        W(dst, 0x10) = W(src, 0x10);
        src += 4;
        dst += 4;
        words++;
    }

    H(dst, 0x10) = (unsigned short)(H(src, 0x10) + D_800F1070);
    H(dst, 0x12) = (unsigned short)(H(src, 0x12) + D_800F1072);

    if (is_gouraud) {
        W(dst, 0x14) = W(src, 0x14);
        src += 4;
        dst += 4;
        words++;
    }

    if (is_quad) {
        if (has_tex) {
            W(dst, 0x14) = W(src, 0x14);
            src += 4;
            dst += 4;
            words++;
        }

        H(dst, 0x14) = (unsigned short)(H(src, 0x14) + D_800F1070);
        H(dst, 0x16) = (unsigned short)(H(src, 0x16) + D_800F1072);
        words++;

        if (is_gouraud) {
            W(dst, 0x18) = W(src, 0x18);
            words++;
        }
    }

    D_800EF8A8 = func_800B3578(D_800EF8A8, arg1, arg2, words);
}
