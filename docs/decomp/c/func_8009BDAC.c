/*
 * func_8009BDAC
 *
 * Purpose:
 *   Initializes 2 double-buffered sets of 48 POLY_FT4 GPU primitives (24 per
 *   team/group) in the primitive buffer pointed to by Scratchpad+0x190
 *   (offset +0x2A30, 40-byte stride, 0x3AC0 buffer stride). Sets tpage (0x1C0),
 *   CLUT via func_80050308(Scratchpad+0x2DD / +0x2DE, 0), UV coordinates from
 *   D_800CB39C (with u += 0x40 for indices 24..47, v += 0x80), and neutral RGB
 *   (0x80, 0x80, 0x80).
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Global fields read:
 *   D_800CB39C (u8[8][8] UV offset table for 8 variants per group)
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x190 (u8 *) double-buffered GPU primitive buffer base
 *   +0x2DD (u8) CLUT index for primitives 0..23
 *   +0x2DE (u8) CLUT index for primitives 24..47
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800CB39C[];

extern void func_800ADD6C(unsigned char *poly);
extern void func_800ADCB8(unsigned char *poly, unsigned int semi);
extern unsigned short func_800ADAD8(int tp, int abr, int x, int y);
extern unsigned short func_80050308(unsigned char idx, unsigned int arg1);

void func_8009BDAC(void)
{
    int buf;
    int buf_off = 0;

    for (buf = 0; buf < 2; buf++, buf_off += 0x3AC0) {
        int i;
        int poly_off = 0x2A30;

        for (i = 0; i < 48; i++, poly_off += 0x28) {
            int sub = i % 8;
            unsigned char *poly = (unsigned char *)W(Scratchpad, 0x190) + buf_off + poly_off;
            unsigned char u_base;
            unsigned char clut_arg;
            const unsigned char *uv;

            func_800ADD6C(poly);
            func_800ADCB8(poly, 0);
            H(poly, 0x16) = func_800ADAD8(0, 0, 0x1C0, 0);

            if (i < 0x18) {
                clut_arg = B(Scratchpad, 0x2DD);
                u_base = 0;
            } else {
                clut_arg = B(Scratchpad, 0x2DE);
                u_base = 0x40;
            }

            H(poly, 0x0E) = func_80050308(clut_arg, 0);

            uv = &D_800CB39C[sub * 8];
            B(poly, 0x0C) = (unsigned char)(uv[0] + u_base);
            B(poly, 0x0D) = (unsigned char)(uv[1] + 0x80);
            B(poly, 0x14) = (unsigned char)(uv[2] + u_base);
            B(poly, 0x15) = (unsigned char)(uv[3] + 0x80);
            B(poly, 0x1C) = (unsigned char)(uv[4] + u_base);
            B(poly, 0x1D) = (unsigned char)(uv[5] + 0x80);
            B(poly, 0x24) = (unsigned char)(uv[6] + u_base);
            B(poly, 0x25) = (unsigned char)(uv[7] + 0x80);

            B(poly, 0x04) = 0x80;
            B(poly, 0x05) = 0x80;
            B(poly, 0x06) = 0x80;
        }
    }
}
