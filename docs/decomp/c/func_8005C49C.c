/*
 * func_8005C49C
 *
 * Purpose:
 *   Initializes an 8x5 grid of 40 double-buffered POLY_GT4 primitives (for two
 *   sides/targets per frame buffer) in the primitive pool pointed to by
 *   Scratchpad+0x190, along with their 3D SVECTOR grid vertices in D_800FF748+0x83A4
 *   and transform/offset parameters D_8010A36C..D_8010A388 (likely goal net or
 *   cloth mesh).
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Global / struct fields written:
 *   D_8010A36C (u32) cleared to 0
 *   D_8010A370..D_8010A388 (s16) initialized to {-0x40, 0x40, 0, 0, 0xA0, 0xA0, -0x30, 0, 0, -0x30, 0, 0}
 *   D_800FF748 + 0xAC20[0..1] (u8) set to 1
 *   D_800FF748 + 0xAC44[0..1] (s16) set to 0x1000
 *   *(u8 **)Scratchpad+0x190 + buf*0x3AC0 + (0x13B0 + side*0x820) + i*0x34:
 *     POLY_GT4 initialized (tpage at +0x1A with tex (0x1C0, 0), UV grid at +0x0C/+0x18/+0x24/+0x30)
 *   D_800FF748 + 0x83A4 + i*0x20 (4 SVECTORs per quad, 8-byte stride, z=0)
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x190 (u8 *) base pointer to primitive/packet buffer
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   8x5 grid (40 quads), texture page (0x1C0, 0), UV step 8 (7 on last col/row),
 *   vertex grid step 8 spanning [-0x20 .. +0x20] x [-0x20 .. +0x08] with z = 0.
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned int D_8010A36C;
extern short D_8010A370;
extern short D_8010A372;
extern short D_8010A374;
extern short D_8010A376;
extern short D_8010A378;
extern short D_8010A37A;
extern short D_8010A37C;
extern short D_8010A37E;
extern short D_8010A380;
extern short D_8010A384;
extern short D_8010A386;
extern short D_8010A388;

extern void func_800ADD94(unsigned char *poly);
extern void func_800ADCE0(unsigned char *poly, int semi);
extern void func_800ADCB8(unsigned char *poly, int shade);
extern unsigned short func_800ADAD8(int tp, int abr, int x, int y);

void func_8005C49C(void)
{
    int buf;
    int side;
    int i;
    int buf_off = 0;
    unsigned char *vtx_cursor;

    D_8010A370 = -0x40;
    D_8010A372 = 0x40;
    D_8010A37A = 0xA0;
    D_8010A378 = 0xA0;
    D_8010A36C = 0;
    D_8010A376 = 0;
    D_8010A374 = 0;
    D_8010A384 = -0x30;
    D_8010A37C = -0x30;
    D_8010A386 = 0;
    D_8010A37E = 0;
    D_8010A388 = 0;
    D_8010A380 = 0;

    for (buf = 0; buf < 2; buf++) {
        int side_off = 0x13B0;

        B(D_800FF748, 0xAC20 + buf) = 1;
        S(D_800FF748, 0xAC44 + buf * 2) = 0x1000;

        for (side = 0; side < 2; side++) {
            int poly_off = 0;
            int u_shift = side << 6;

            for (i = 0; i < 0x28; i++) {
                unsigned char *poly = *(unsigned char **)(Scratchpad + 0x190) + buf_off + side_off + poly_off;
                int row;
                int row8;
                int col;
                int w_uv;
                int h_uv;
                int u0;
                int v0;

                func_800ADD94(poly);
                func_800ADCE0(poly, 0);
                func_800ADCB8(poly, 0);
                H(poly, 0x1A) = func_800ADAD8(0, 0, 0x1C0, 0);

                row = i / 8;
                row8 = row * 8;
                col = i - row8;
                w_uv = (col == 7) ? 7 : 8;
                h_uv = (row == 4) ? 7 : 8;

                u0 = (col * 8) + u_shift;
                v0 = row8 - 0x50;

                B(poly, 0x0C) = (unsigned char)u0;
                B(poly, 0x0D) = (unsigned char)v0;
                B(poly, 0x18) = (unsigned char)(u0 + w_uv);
                B(poly, 0x19) = (unsigned char)v0;
                B(poly, 0x24) = (unsigned char)u0;
                B(poly, 0x25) = (unsigned char)(v0 + h_uv);
                B(poly, 0x30) = (unsigned char)(u0 + w_uv);
                B(poly, 0x31) = (unsigned char)(v0 + h_uv);

                poly_off += 0x34;
            }
            side_off += 0x820;
        }
        buf_off += 0x3AC0;
    }

    vtx_cursor = D_800FF748;
    for (i = 0; i < 0x28; i++) {
        int row8 = (i / 8) * 8;
        int col8 = (i - row8) * 8;
        short x0 = (short)(col8 - 0x20);
        short y0 = (short)(row8 - 0x20);
        short x1 = (short)(col8 - 0x18);
        short y1 = (short)(row8 - 0x18);

        S(vtx_cursor, 0x83A4) = x0;
        S(vtx_cursor, 0x83A6) = y0;
        S(vtx_cursor, 0x83A8) = 0;

        S(vtx_cursor, 0x83AC) = x1;
        S(vtx_cursor, 0x83AE) = y0;
        S(vtx_cursor, 0x83B0) = 0;

        S(vtx_cursor, 0x83B4) = x0;
        S(vtx_cursor, 0x83B6) = y1;
        S(vtx_cursor, 0x83B8) = 0;

        S(vtx_cursor, 0x83BC) = x1;
        S(vtx_cursor, 0x83BE) = y1;
        S(vtx_cursor, 0x83C0) = 0;

        vtx_cursor += 0x20;
    }
}
