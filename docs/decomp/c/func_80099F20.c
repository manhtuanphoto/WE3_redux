/*
 * func_80099F20
 *
 * Purpose:
 *   Clears 6 state bytes at D_800CAA70..D_800CAA75, adjusts VRAM Y/offset entries
 *   in D_800CA96C based on Scratchpad+0x2E3, and initializes 2x2x20 = 80 POLY_FT4
 *   primitives in the primitive buffer at *(Scratchpad+0x190) (setting TPage,
 *   CLUT, UVs from D_800CA8DC, and neutral RGB 0x80,0x80,0x80).
 *
 * Arguments:
 *   mode: (unsigned char) selects CLUT Y (0x1FF vs 0x1FE) when entry type == 2
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Scratchpad (0x1F800000):
 *     +0x190 (u32) base address of GPU primitive buffer (R)
 *     +0x2E3 (u8)  mode flag (== 3 shifts D_800CA96C entries +0x80, else -0x80) (R)
 *   D_800CAA70..D_800CAA75: 6 bytes cleared to 0 (W)
 *   D_800CA96C: 33 entries (stride 8 bytes), u16 at +0x00 adjusted +/-0x80 (R/W)
 *   D_800CA904[20]: entry type table (0, 1, or 2) (R)
 *   D_800CA8DC: 8-byte UV coordinate table per entry type (R)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800CAA75;
extern unsigned char D_800CA96C[];
extern unsigned char D_800CA904[];
extern unsigned char D_800CA8DC[];

extern void func_800ADD6C(unsigned char *prim);                  /* setPolyFT4 */
extern void func_800ADCB8(unsigned char *prim, int semi_trans);  /* setSemiTrans */
extern unsigned short func_800ADAD8(int tp, int abr, int x, int y); /* getTPage */
extern unsigned short func_800ADB14(int x, int y);               /* getClut */

void func_80099F20(unsigned char mode) {
    int i;
    int buf_idx;
    int half_idx;
    int buf_off;

    for (i = 0; i < 6; i++) {
        *(&D_800CAA75 - i) = 0;
    }

    if (B(Scratchpad, 0x2E3) == 3) {
        if (S(D_800CA96C, 0) == 0x3A98) {
            for (i = 0; i < 0x21; i++) {
                H(D_800CA96C, i * 8) = (unsigned short)(H(D_800CA96C, i * 8) + 0x80);
            }
        }
    } else {
        if (S(D_800CA96C, 0) != 0x3A98) {
            for (i = 0; i < 0x21; i++) {
                H(D_800CA96C, i * 8) = (unsigned short)(H(D_800CA96C, i * 8) - 0x80);
            }
        }
    }

    buf_off = 0;
    for (buf_idx = 0; buf_idx < 2; buf_idx++) {
        int block_off = 0x23F0;
        for (half_idx = 0; half_idx < 2; half_idx++) {
            int prim_off = 0;
            for (i = 0; i < 0x14; i++) {
                unsigned char *prim = (unsigned char *)(W(Scratchpad, 0x190) + buf_off + block_off + prim_off);
                unsigned char entry_type;
                int tp = 0;
                int abr = 0;
                signed short tpage_x = 0; /* UNCLEAR: left uninitialized in asm if entry_type > 2 */
                signed short tpage_y = 0; /* UNCLEAR: left uninitialized in asm if entry_type > 2 */
                signed short clut_x = 0;  /* UNCLEAR: left uninitialized in asm if entry_type > 2 */
                signed short clut_y = 0;  /* UNCLEAR: left uninitialized in asm if entry_type > 2 */
                unsigned short tpage;
                unsigned short clut;
                unsigned char *uv;

                func_800ADD6C(prim);
                func_800ADCB8(prim, 0);

                entry_type = D_800CA904[i];
                if (entry_type == 0) {
                    tpage_x = 0x340;
                    tpage_y = 0x180;
                    tp = 1;
                    clut_x = 0;
                    clut_y = 0x1FD;
                } else if (entry_type == 1) {
                    tpage_x = 0x340;
                    tpage_y = 0x180;
                    tp = 1;
                    clut_x = 0;
                    clut_y = 0x1FE;
                } else if (entry_type == 2) {
                    tpage_x = 0x340;
                    tpage_y = 0x180;
                    tp = 1;
                    clut_x = 0;
                    clut_y = (mode == 0) ? 0x1FF : 0x1FE;
                }

                tpage = func_800ADAD8(tp, abr, tpage_x, tpage_y);
                H(prim, 0x16) = tpage;
                clut = func_800ADB14(clut_x, clut_y);
                H(prim, 0x0E) = clut;

                uv = D_800CA8DC + (entry_type << 3);
                B(prim, 0x0C) = uv[0];
                B(prim, 0x0D) = (unsigned char)(uv[1] + 0x80);
                B(prim, 0x14) = uv[2];
                B(prim, 0x15) = (unsigned char)(uv[3] + 0x80);
                B(prim, 0x1C) = uv[4];
                B(prim, 0x1D) = (unsigned char)(uv[5] + 0x80);
                B(prim, 0x24) = uv[6];
                B(prim, 0x25) = (unsigned char)(uv[7] + 0x80);

                B(prim, 0x04) = 0x80;
                B(prim, 0x05) = 0x80;
                B(prim, 0x06) = 0x80;

                prim_off += 0x28;
            }
            block_off += 0x320;
        }
        buf_off += 0x3AC0;
    }
}
