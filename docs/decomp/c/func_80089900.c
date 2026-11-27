/*
 * func_80089900 - Record per-frame replay snapshot for 22 players and the ball
 *
 * Purpose:
 *   Copies active player states (id low byte, animation frame, body heading,
 *   x/y/z coordinates, foot/mirror flag) for up to 22 players (D_800FF748 + 1..22)
 *   and the ball's x/y/z coordinates plus 8 bytes of transform/rotation data
 *   into 120-frame circular replay buffers indexed by Scratchpad 0x288, then
 *   advances Scratchpad 0x288 modulo 120.
 *
 * Arguments:
 *   None (void).
 *
 * Return value:
 *   None (void).
 *
 * Scratchpad / Globals read & written:
 *   Scratch 0x288 (s16) - replay ring-buffer frame index (0..119, incremented mod 120)
 *   Scratch 0x2F4 (u8*) - pointer to ball actor struct
 *   D_800FF748    (u8*) - actor array (stride 0x3E8 = 1000 bytes; loop checks indices 1..22)
 *   D_800F2B10[120][22] (u8)  - recorded player actor_id low byte (Actor +0x38E)
 *   D_800F0450[120][22] (u8)  - recorded player anim frame (Actor +0x390)
 *   D_800F20B8[120][22] (u8)  - recorded player body heading (Actor +0x3AA)
 *   D_800EF9D0[120][22] (u8)  - recorded player mirror/foot flag (Actor +0x3AC)
 *   D_800FB6C0[120][22] (u16) - recorded player x position (Actor +0x02)
 *   D_800FCC50[120][22] (u16) - recorded player y position (Actor +0x04)
 *   D_800FE1E0[120][22] (u16) - recorded player z position (Actor +0x06)
 *   D_800FB5D0[120]     (u16) - recorded ball x position (Ball +0x02)
 *   D_800FCB60[120]     (u16) - recorded ball y position (Ball +0x04)
 *   D_800FE0F0[120]     (u16) - recorded ball z position (Ball +0x06)
 *   D_800E7DF8[120][2]  (u32) - recorded 8-byte ball data from Ball +0x24..+0x2B (unaligned)
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned char D_800F2B10[];
extern unsigned char D_800F0450[];
extern unsigned char D_800F20B8[];
extern unsigned char D_800EF9D0[];
extern unsigned short D_800FB6C0[];
extern unsigned short D_800FCC50[];
extern unsigned short D_800FE1E0[];
extern unsigned short D_800FB5D0[];
extern unsigned short D_800FCB60[];
extern unsigned short D_800FE0F0[];
extern unsigned int D_800E7DF8[];

void func_80089900(void)
{
    unsigned char *player = D_800FF748 + 0x3E8;
    unsigned char *ball;
    int i;
    int idx;

    for (i = 0; i < 22; i++, player += 0x3E8) {
        if (B(player, 0x00) == 0) {
            continue;
        }

        idx = S(Scratchpad, 0x288);
        D_800F2B10[idx * 22 + i] = (unsigned char)H(player, 0x38E);

        idx = S(Scratchpad, 0x288);
        D_800F0450[idx * 22 + i] = B(player, 0x390);

        idx = S(Scratchpad, 0x288);
        D_800F20B8[idx * 22 + i] = B(player, 0x3AA);

        idx = S(Scratchpad, 0x288);
        D_800FB6C0[idx * 22 + i] = H(player, 0x02);

        idx = S(Scratchpad, 0x288);
        D_800FCC50[idx * 22 + i] = H(player, 0x04);

        idx = S(Scratchpad, 0x288);
        D_800FE1E0[idx * 22 + i] = H(player, 0x06);

        idx = S(Scratchpad, 0x288);
        D_800EF9D0[idx * 22 + i] = B(player, 0x3AC);
    }

    idx = S(Scratchpad, 0x288);
    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    D_800FB5D0[idx] = H(ball, 0x02);

    idx = S(Scratchpad, 0x288);
    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    D_800FCB60[idx] = H(ball, 0x04);

    idx = S(Scratchpad, 0x288);
    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    D_800FE0F0[idx] = H(ball, 0x06);

    idx = S(Scratchpad, 0x288);
    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    /* Original MIPS uses lwl/lwr and swl/swr (unaligned 32-bit copy of Ball +0x24..+0x2B). */
    D_800E7DF8[idx * 2] = W(ball, 0x24);
    D_800E7DF8[idx * 2 + 1] = W(ball, 0x28);

    S(Scratchpad, 0x288) = (signed short)((S(Scratchpad, 0x288) + 1) % 120);
}
