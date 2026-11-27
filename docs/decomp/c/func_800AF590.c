/*
 * func_800AF590
 *
 * Purpose:
 *   Builds a PS1 GPU DR_ENV primitive packet from a DRAWENV-like struct (PsyQ
 *   SetDrawEnv equivalent). Encodes draw-area top-left (E3), bottom-right (E4),
 *   draw offset (E5), texpage/mode (E1), texture window (E2), and mask setting (E6).
 *   When env->isbg (+0x18) is nonzero, clamps w/h to D_800CEAC8/D_800CEACA and
 *   appends either a 0x02 VRAM Fill (if x and w are multiples of 64) or a 0x60
 *   TILE primitive (offset-adjusted), then sets the packet word count at prim+0x3.
 *
 * Arguments:
 *   prim (unsigned char *): output DR_ENV GPU primitive packet buffer
 *   env (unsigned char *): input DRAWENV struct:
 *     +0x00..+0x07: clip RECT { s16 x, y, w, h }
 *     +0x08..+0x0B: ofs[2] { s16 ox, oy }
 *     +0x0C..+0x13: tw RECT (texture window)
 *     +0x14 (u16) tpage, +0x16 (u8) dtd, +0x17 (u8) dfe,
 *     +0x18 (u8) isbg, +0x19 (u8) r0, +0x1A (u8) g0, +0x1B (u8) b0
 *
 * Return value:
 *   None (void)
 *
 * Global fields read:
 *   D_800CEAC8 (s16/u16) VRAM width limit,
 *   D_800CEACA (s16/u16) VRAM height limit
 *
 * Global fields written:
 *   None
 */

#define B(p, o) (*(unsigned char *)((p) + (o)))
#define H(p, o) (*(unsigned short *)((p) + (o)))
#define S(p, o) (*(signed short *)((p) + (o)))
#define W(p, o) (*(unsigned int *)((p) + (o)))

extern signed short D_800CEAC8;
extern signed short D_800CEACA;

extern unsigned int func_800AF820(signed short x, signed short y);
extern unsigned int func_800AF8B8(signed short x2, signed short y2);
extern unsigned int func_800AF950(signed short ox, signed short oy);
extern unsigned int func_800AF800(unsigned char *tw, unsigned char dtd, unsigned short tpage);
extern unsigned int func_800AF96C(unsigned char *tw);

void func_800AF590(unsigned char *prim, unsigned char *env)
{
    int idx;

    W(prim, 0x04) = func_800AF820(S(env, 0), S(env, 2));

    {
        signed short x2 = (signed short)(H(env, 0) + H(env, 4) - 1);
        signed short y2 = (signed short)(H(env, 2) + H(env, 6) - 1);
        W(prim, 0x08) = func_800AF8B8(x2, y2);
    }

    W(prim, 0x0C) = func_800AF950(S(env, 8), S(env, 0xA));

    /*
     * UNCLEAR: asm loads lbu $a0, 0x17($s0) (env->dfe) before jal func_800AF800,
     * but overwrites $a0 with ($s0 + 0xC) in the jal delay slot.
     */
    W(prim, 0x10) = func_800AF800(env + 0xC, B(env, 0x16), H(env, 0x14));
    W(prim, 0x14) = func_800AF96C(env + 0xC);
    W(prim, 0x18) = 0xE6000000U;
    idx = 7;

    if (B(env, 0x18) != 0) {
        unsigned short rect[4];
        signed short w_s;
        signed short h_s;

        rect[0] = H(env, 0x0);
        rect[1] = H(env, 0x2);
        rect[2] = H(env, 0x4);
        rect[3] = H(env, 0x6);

        w_s = (signed short)rect[2];
        if (w_s < 0) {
            rect[2] = 0;
        } else if ((int)D_800CEAC8 - 1 < (int)w_s) {
            rect[2] = (unsigned short)((unsigned short)D_800CEAC8 - 1);
        }

        h_s = (signed short)rect[3];
        if (h_s < 0) {
            rect[3] = 0;
        } else if ((int)D_800CEACA - 1 < (int)h_s) {
            rect[3] = (unsigned short)((unsigned short)D_800CEACA - 1);
        }

        if ((rect[0] & 0x3F) != 0 || (rect[2] & 0x3F) != 0) {
            int i0 = idx;
            int i1 = idx + 1;
            int i2 = idx + 2;
            idx += 3;

            rect[0] = (unsigned short)(rect[0] - H(env, 0x8));
            rect[1] = (unsigned short)(rect[1] - H(env, 0xA));

            W(prim, i0 << 2) = 0x60000000U
                             | ((unsigned int)B(env, 0x1B) << 16)
                             | ((unsigned int)B(env, 0x1A) << 8)
                             | (unsigned int)B(env, 0x19);
            W(prim, i1 << 2) = ((unsigned int)rect[1] << 16) | rect[0];
            W(prim, i2 << 2) = ((unsigned int)rect[3] << 16) | rect[2];
        } else {
            int i0 = idx;
            int i1 = idx + 1;
            int i2 = idx + 2;
            idx += 3;

            W(prim, i0 << 2) = 0x02000000U
                             | ((unsigned int)B(env, 0x1B) << 16)
                             | ((unsigned int)B(env, 0x1A) << 8)
                             | (unsigned int)B(env, 0x19);
            W(prim, i1 << 2) = ((unsigned int)rect[1] << 16) | rect[0];
            W(prim, i2 << 2) = ((unsigned int)rect[3] << 16) | rect[2];
        }
    }

    B(prim, 0x03) = (unsigned char)(idx - 1);
}
