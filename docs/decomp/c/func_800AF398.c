/*
 * func_800AF398
 *
 * Purpose:
 *   PS1 PsyQ SetDrawEnv equivalent: builds a GPU DR_ENV primitive packet in
 *   `dr_env` from a DRAWENV descriptor `env`. Encodes draw area top-left (GP0 0xE3),
 *   bottom-right (GP0 0xE4), draw offset (GP0 0xE5), texture page/mode, texture
 *   window (GP0 0xE2), mask setting (GP0 0xE6), and optionally an opaque fill
 *   TILE (GP0 0x60) clamped to VRAM bounds (D_800CEAC8 x D_800CEACA) when
 *   env->isbg != 0. Writes the packet word count into dr_env[3] (byte).
 *
 * Arguments:
 *   dr_env - pointer to output GPU DR_ENV packet (u32 words; byte +3 = word len)
 *   env    - pointer to input DRAWENV structure
 *
 * Return value:
 *   None (void)
 *
 * Global fields read:
 *   D_800CEAC8 (s16/u16) VRAM width limit (e.g., 1024)
 *   D_800CEACA (s16/u16) VRAM height limit (e.g., 512)
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define SH(p, o) (*(short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))

extern short D_800CEAC8;
extern short D_800CEACA;

extern unsigned int func_800AF820(short x, short y);
extern unsigned int func_800AF8B8(short x, short y);
extern unsigned int func_800AF950(short ox, short oy);
extern unsigned int func_800AF800(const unsigned char *tw, unsigned char dtd, unsigned short tpage);
extern unsigned int func_800AF96C(void);

void func_800AF398(unsigned char *dr_env, const unsigned char *env)
{
    int idx;
    short x1, y1;

    W(dr_env, 0x04) = func_800AF820(SH(env, 0x00), SH(env, 0x02));

    x1 = (short)((int)H(env, 0x04) + (int)H(env, 0x00) - 1);
    y1 = (short)((int)H(env, 0x06) + (int)H(env, 0x02) - 1);
    W(dr_env, 0x08) = func_800AF8B8(x1, y1);

    W(dr_env, 0x0C) = func_800AF950(SH(env, 0x08), SH(env, 0x0A));

    /*
     * Note: assembly executes `lbu $a0, 0x17($s0)` (env->dfe) before
     * overwriting `$a0` with `env + 0xC` in the delay slot of `jal func_800AF800`.
     * Similarly, `func_800AF96C` immediately follows without reloading `$a0`.
     */
    W(dr_env, 0x10) = func_800AF800(env + 0x0C, B(env, 0x16), H(env, 0x14));
    W(dr_env, 0x14) = func_800AF96C();
    W(dr_env, 0x18) = 0xE6000000U;

    idx = 7;
    if (B(env, 0x18) != 0) {
        unsigned short clip[4];
        short w, h;
        unsigned int cmd;

        clip[0] = H(env, 0x00);
        clip[1] = H(env, 0x02);
        clip[2] = H(env, 0x04);
        clip[3] = H(env, 0x06);

        w = (short)clip[2];
        if (w < 0) {
            clip[2] = 0;
        } else if (w > (short)(D_800CEAC8 - 1)) {
            clip[2] = (unsigned short)((unsigned short)D_800CEAC8 - 1);
        }

        h = (short)clip[3];
        if (h < 0) {
            clip[3] = 0;
        } else if (h > (short)(D_800CEACA - 1)) {
            clip[3] = (unsigned short)((unsigned short)D_800CEACA - 1);
        }

        clip[0] = (unsigned short)(clip[0] - H(env, 0x08));
        clip[1] = (unsigned short)(clip[1] - H(env, 0x0A));

        cmd = 0x60000000U |
              ((unsigned int)B(env, 0x1B) << 16) |
              ((unsigned int)B(env, 0x1A) << 8) |
              (unsigned int)B(env, 0x19);
        W(dr_env, idx * 4) = cmd;
        idx++;
        W(dr_env, idx * 4) = ((unsigned int)clip[1] << 16) | (unsigned int)clip[0];
        idx++;
        W(dr_env, idx * 4) = ((unsigned int)clip[3] << 16) | (unsigned int)clip[2];
        idx++;
    }

    B(dr_env, 0x03) = (unsigned char)(idx - 1);
}
