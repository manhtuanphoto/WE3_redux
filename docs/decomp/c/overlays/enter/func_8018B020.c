/*
 * func_8018B020 (enter overlay)
 *
 * Purpose:
 *   Initializes the 8x5 (40-tile) 3D curved national flag / banner mesh
 *   (D_800FF748 + 0x83A4, stride 0x20) and its double-buffered PS1 POLY_GT4
 *   primitive templates in the active GPU packet buffer (Scratchpad+0x190) for
 *   both teams during the pre-match entrance sequence.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Global fields read / written:
 *   D_8010A36C (s32), D_8010A370..D_8010A388 (s16) - banner/camera transform params
 *   D_8010C1D8 + 0x0D[2] (u8)  - cleared to 0 per buffer
 *   D_8010865D (u8)            - active draw buffer parity (XORed with team index)
 *   D_800FF748 + 0xAC20[2] (u8)  - set to 1 per buffer
 *   D_800FF748 + 0xAC44[2] (s16) - set to 0x1000 per buffer
 *   D_800FF748 + 0x83A4[40] (stride 0x20) - 8x5 tile 3D corner vertices:
 *     +0x00/+0x02/+0x04 (s16) top-left (x, y, z)
 *     +0x08/+0x0A/+0x0C (s16) top-right (x, y, z)
 *     +0x10/+0x12/+0x14 (s16) bottom-left (x, y, z)
 *     +0x18/+0x1A/+0x1C (s16) bottom-right (x, y, z)
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x190 (u32) - base pointer to GPU primitive buffer (POLY_GT4 stride 0x34)
 *
 * PS1 GPU primitive details:
 *   - 40 POLY_GT4 primitives per team (8x5 tiles covering 64x40 texture at
 *     VRAM (448, 0) 4-bit CLUT, v = 0xB0..0xD7, u = 0..63 or 64..127; rightmost
 *     column uses width 7 and bottom row uses height 7).
 */

#define B(p, o) (*(unsigned char *)((p) + (o)))
#define H(p, o) (*(unsigned short *)((p) + (o)))
#define S(p, o) (*(signed short *)((p) + (o)))
#define W(p, o) (*(unsigned int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned char D_8010865D;
extern signed int D_8010A36C;
extern signed short D_8010A370;
extern signed short D_8010A372;
extern signed short D_8010A374;
extern signed short D_8010A376;
extern signed short D_8010A378;
extern signed short D_8010A37A;
extern signed short D_8010A37C;
extern signed short D_8010A37E;
extern signed short D_8010A380;
extern signed short D_8010A384;
extern signed short D_8010A386;
extern signed short D_8010A388;
extern unsigned char D_8010C1D8[];

extern void func_800ADD94(void *poly); /* SetPolyGT4 (code=0x3C, len=0xC) */
extern void func_800ADCE0(void *poly, int shade_tex); /* SetShadeTex */
extern void func_800ADCB8(void *poly, int semi_trans); /* SetSemiTrans */
extern unsigned short func_800ADAD8(int tp, int abr, int x, int y); /* GetTPage */

void func_8018B020(void)
{
    static const signed short col_z[9] = { 0, 0x1A, 0x2D, 0x38, 0x3C, 0x38, 0x2D, 0x1A, 0 };
    static const signed short row_z[6] = { 0, 0x13, 0x1C, 0x1C, 0x13, 0 };

    int buf_idx;
    int buf_offset = 0;
    int tile;

    /* Note: assembly first writes -0x2100/-0x2900 to D_8010A378/D_8010A37A,
     * then immediately overwrites them with -0x2180/-0x2880. */
    D_8010A378 = -0x2180;
    D_8010A37A = -0x2880;
    D_8010A384 = -0x400;
    D_8010A37C = -0x400;
    D_8010A36C = 0;
    D_8010A370 = 0;
    D_8010A372 = 0;
    D_8010A376 = 0;
    D_8010A374 = 0;
    D_8010A386 = 0x400;
    D_8010A37E = 0x400;
    D_8010A388 = 0;
    D_8010A380 = 0;

    /* Initialize double-buffered 8x5 POLY_GT4 flag tiles for both teams */
    for (buf_idx = 0; buf_idx < 2; buf_idx++) {
        int team;

        B(D_8010C1D8, 0xD + buf_idx) = 0;
        B(D_800FF748, 0xAC20 + buf_idx) = 1;
        S(D_800FF748, 0xAC44 + buf_idx * 2) = 0x1000;

        for (team = 0; team < 2; team++) {
            int u_base = team << 6;
            int poly_off = 0;

            for (tile = 0; tile < 0x28; tile++, poly_off += 0x34) {
                unsigned int side_xor = (unsigned int)team ^ (unsigned int)D_8010865D;
                unsigned char *poly = (unsigned char *)(W(Scratchpad, 0x190) + buf_offset + (int)(side_xor * 0x820U) + 0x13B0 + poly_off);
                int row = tile / 8;
                int col = tile - row * 8;
                int w = (col == 7) ? 7 : 8;
                int h = (row == 4) ? 7 : 8;
                unsigned char u0 = (unsigned char)((col << 3) + u_base);
                unsigned char v0 = (unsigned char)((row << 3) - 0x50);
                unsigned char u1 = (unsigned char)(u0 + w);
                unsigned char v1 = (unsigned char)(v0 + h);

                func_800ADD94(poly);
                func_800ADCE0(poly, 0);
                func_800ADCB8(poly, 0);
                H(poly, 0x1A) = func_800ADAD8(0, 0, 0x1C0, 0);

                B(poly, 0x0C) = u0;
                B(poly, 0x0D) = v0;
                B(poly, 0x18) = u1;
                B(poly, 0x19) = v0;
                B(poly, 0x24) = u0;
                B(poly, 0x25) = v1;
                B(poly, 0x30) = u1;
                B(poly, 0x31) = v1;
            }

            buf_offset += 0x3AC0;
        }
    }

    /* Build 8x5 3D curved mesh vertex grid (x in [-600..600], y in [-375..375]) */
    for (tile = 0; tile < 0x28; tile++) {
        unsigned char *entry = D_800FF748 + 0x83A4 + tile * 0x20;
        int row = tile / 8;
        int col = tile - row * 8;
        int x_left = col * 150 - 0x258;
        int x_right = col * 150 - 0x1C2;
        int y_top = row * 150 - 0x177;
        int y_bot = row * 150 - 0xE1;
        signed short z_left = (col < 8) ? col_z[col] : 0;
        signed short z_right = (col < 8) ? col_z[col + 1] : 0;

        /* Note: assembly switches on col (jtbl_80189264) and then row
         * (jtbl_80189284/9C/B4/CC), with all 4 column groups applying the same
         * row_z arch offsets. */
        signed short z_top = (col < 8 && row < 5) ? row_z[row] : 0;
        signed short z_bot = (col < 8 && row < 5) ? row_z[row + 1] : 0;

        S(entry, 0x00) = (signed short)x_left;
        S(entry, 0x02) = (signed short)y_top;
        S(entry, 0x04) = (signed short)(z_left + z_top);

        S(entry, 0x08) = (signed short)x_right;
        S(entry, 0x0A) = (signed short)y_top;
        S(entry, 0x0C) = (signed short)(z_right + z_top);

        S(entry, 0x10) = (signed short)x_left;
        S(entry, 0x12) = (signed short)y_bot;
        S(entry, 0x14) = (signed short)(z_left + z_bot);

        S(entry, 0x18) = (signed short)x_right;
        S(entry, 0x1A) = (signed short)y_bot;
        S(entry, 0x1C) = (signed short)(z_right + z_bot);
    }
}
