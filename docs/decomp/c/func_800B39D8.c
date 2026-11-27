/*
 * func_800B39D8 - Emit a 2D PS1 GPU primitive (SPRT or transformed POLY_FT4)
 *
 * Purpose:
 *   Reads a sprite/object descriptor at `desc` and builds either a PS1 GPU
 *   textured sprite (SPRT, command 0x64, 5 words) when unrotated, unit-scaled
 *   (0x1000, 0x1000), and unflipped (or bit 27 forced), or a rotated/scaled/
 *   flipped textured quad (POLY_FT4, command 0x2C, 9 words) transformed via
 *   GTE matrix helpers. Submits the packet into the ordering table via
 *   func_800B3578 and advances D_800EF8A8.
 *
 * Arguments:
 *   desc (unsigned char *) - pointer to sprite descriptor struct:
 *     +0x00 u32 flags (bit 31 = hidden, bit 27 = force SPRT, bit 23 = H-flip,
 *                      bit 22 = V-flip, bits 6..8/25..30 = GPU tpage/blend/raw)
 *     +0x04 s16 x, +0x06 s16 y
 *     +0x08 u16 width, +0x0A u16 height
 *     +0x0C u16 tpage_x
 *     +0x0E u8  u, +0x0F u8 v
 *     +0x10 s16 clut_x, +0x12 s16 clut_y
 *     +0x14 u8  r, +0x15 u8 g, +0x16 u8 b
 *     +0x18 s16 pivot_x, +0x1A s16 pivot_y
 *     +0x1C s16 scale_x, +0x1E s16 scale_y (0x1000 = 1.0)
 *     +0x20 s32 rotation (in 1/360-scaled units; divided by 360)
 *   ot_ptr (void *)        - ordering table entry pointer passed to func_800B3578
 *   ot_slot (unsigned int) - 16-bit OT slot/depth passed to func_800B3578
 *
 * Return value:
 *   None (void).
 *
 * Global fields read & written:
 *   D_800EF8A8 (unsigned char *) - current GPU packet buffer pointer (updated)
 *   D_800F1070 (s16)             - global screen X offset
 *   D_800F1072 (s16)             - global screen Y offset
 *   D_800FB5B0 (u32[8])          - identity rotation/scale matrix (32 bytes)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x10001000 = unit scale (4096, 4096 in 4.12 fixed point).
 *   0x64000000 = PS1 GPU SPRT command; 0x2C000000 = PS1 GPU POLY_FT4 command.
 *   0xE1000200 = PS1 GPU DR_MODE / TexPage attribute base.
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char *D_800EF8A8;
extern signed short D_800F1070;
extern signed short D_800F1072;
extern unsigned int D_800FB5B0[8];

extern void func_800B1488(signed short *rot, unsigned int *mat);
extern void func_800B1348(unsigned int *mat, signed int *scale);
extern void func_800B1318(unsigned int *mat, signed int *pos);
extern signed int func_800B3F28(void);
extern void func_800B3ED8(unsigned int *mat);
extern void func_800B3F08(unsigned int *mat);
extern void func_800B3F38(
    signed short *v0, signed short *v1, signed short *v2, signed short *v3,
    unsigned int *xy0, unsigned int *xy1, unsigned int *xy2, unsigned int *xy3,
    unsigned int *out4, unsigned int *out5);
extern unsigned char *func_800B3578(unsigned char *pkt, void *ot_ptr, unsigned int ot_slot, int word_count);

void func_800B39D8(unsigned char *desc, void *ot_ptr, unsigned int ot_slot)
{
    unsigned int flags = W(desc, 0x00);
    unsigned int use_sprt;
    unsigned char *pkt;

    if ((signed int)flags < 0) {
        return;
    }
    if (H(desc, 0x08) == 0 || H(desc, 0x0A) == 0) {
        return;
    }

    pkt = D_800EF8A8;
    use_sprt = (flags >> 27) & 1;

    if (W(desc, 0x1C) == 0x10001000) {
        if (W(desc, 0x20) == 0) {
            use_sprt |= ((flags & 0x00C00000) == 0);
        }
    }

    if (use_sprt != 0) {
        signed int sx;
        signed int sy;
        unsigned int clut_uv;
        unsigned int size_wh;

        W(pkt, 0x04) = 0xE1000200 |
                       ((flags >> 17) & 0x180) |
                       ((flags >> 23) & 0x60) |
                       (H(desc, 0x0C) & 0x1F);

        W(pkt, 0x08) = 0x64000000 |
                       ((flags >> 5) & 0x02000000) |
                       ((flags << 18) & 0x01000000) |
                       ((unsigned int)B(desc, 0x16) << 16) |
                       ((unsigned int)B(desc, 0x15) << 8) |
                       B(desc, 0x14);

        sx = (S(desc, 0x04) + D_800F1070) - S(desc, 0x18);
        sy = (S(desc, 0x06) + D_800F1072) - S(desc, 0x1A);
        W(pkt, 0x0C) = (sx & 0xFFFF) | ((unsigned int)sy << 16);

        clut_uv = B(desc, 0x0E) |
                  ((unsigned int)B(desc, 0x0F) << 8) |
                  (((unsigned int)S(desc, 0x10) << 12) & 0x003F0000) |
                  ((unsigned int)S(desc, 0x12) << 22);
        W(pkt, 0x10) = clut_uv;

        size_wh = H(desc, 0x08) | ((unsigned int)H(desc, 0x0A) << 16);
        W(pkt, 0x14) = size_wh;

        D_800EF8A8 = func_800B3578(pkt, ot_ptr, ot_slot & 0xFFFF, 5);
    } else {
        unsigned int mat[8];
        signed short rot[4];
        signed int vec[3];
        signed short v0[4], v1[4], v2[4], v3[4];
        unsigned int xy0, xy1, xy2, xy3, dummy4, dummy5;
        unsigned int u0, u1, v_top, v_bot;
        unsigned int clut_bits;
        unsigned int tpage_bits;
        signed short px, py;
        unsigned short w, h;

        if (W(desc, 0x20) == 0) {
            int i;
            for (i = 0; i < 8; i++) {
                mat[i] = D_800FB5B0[i];
            }
        } else {
            rot[0] = 0;
            rot[1] = 0;
            rot[2] = (signed short)(SW(desc, 0x20) / 360);
            func_800B1488(rot, mat);
        }

        if (W(desc, 0x1C) != 0x10001000) {
            vec[0] = S(desc, 0x1C);
            vec[1] = S(desc, 0x1E);
            vec[2] = 0;
            func_800B1348(mat, vec);
        }

        vec[0] = S(desc, 0x04);
        vec[1] = S(desc, 0x06);
        vec[2] = func_800B3F28();
        func_800B1318(mat, vec);
        func_800B3ED8(mat);
        func_800B3F08(mat);

        px = (signed short)(-S(desc, 0x18));
        py = (signed short)(-S(desc, 0x1A));
        w = H(desc, 0x08);
        h = H(desc, 0x0A);

        v0[0] = px;                 v0[1] = py;                 v0[2] = 0;
        v1[0] = (signed short)(px + w); v1[1] = py;             v1[2] = 0;
        v2[0] = px;                 v2[1] = (signed short)(py + h); v2[2] = 0;
        v3[0] = (signed short)(px + w); v3[1] = (signed short)(py + h); v3[2] = 0;

        func_800B3F38(v0, v1, v2, v3, &xy0, &xy1, &xy2, &xy3, &dummy4, &dummy5);

        if ((flags & 0x00800000) != 0) {
            u0 = (unsigned int)B(desc, 0x0E) + B(desc, 0x08) - 1;
            u1 = B(desc, 0x0E);
        } else {
            u0 = B(desc, 0x0E);
            u1 = (unsigned int)B(desc, 0x0E) + B(desc, 0x08) - 1;
        }

        if ((flags & 0x00400000) != 0) {
            v_top = (unsigned int)B(desc, 0x0F) + B(desc, 0x0A) - 1;
            v_bot = B(desc, 0x0F);
        } else {
            v_top = B(desc, 0x0F);
            v_bot = (unsigned int)B(desc, 0x0F) + B(desc, 0x0A) - 1;
        }

        W(pkt, 0x04) = 0x2C000000 |
                       ((flags >> 5) & 0x02000000) |
                       ((flags << 18) & 0x01000000) |
                       ((unsigned int)B(desc, 0x16) << 16) |
                       ((unsigned int)B(desc, 0x15) << 8) |
                       B(desc, 0x14);

        W(pkt, 0x08) = xy0;
        clut_bits = (((unsigned int)S(desc, 0x10) << 12) & 0x003F0000) |
                    ((unsigned int)S(desc, 0x12) << 22);
        W(pkt, 0x0C) = (u0 & 0xFF) | ((v_top & 0xFF) << 8) | clut_bits;

        W(pkt, 0x10) = xy1;
        tpage_bits = (((unsigned int)H(desc, 0x0C) & 0x1F) << 16) |
                     ((flags >> 1) & 0x01800000) |
                     ((flags >> 7) & 0x00600000);
        W(pkt, 0x14) = (u1 & 0xFF) | ((v_top & 0xFF) << 8) | tpage_bits;

        W(pkt, 0x18) = xy2;
        W(pkt, 0x1C) = (u0 & 0xFF) | ((v_bot & 0xFF) << 8);

        W(pkt, 0x20) = xy3;
        W(pkt, 0x24) = (u1 & 0xFF) | ((v_bot & 0xFF) << 8);

        D_800EF8A8 = func_800B3578(pkt, ot_ptr, ot_slot & 0xFFFF, 9);
    }
}
