/*
 * func_800975A0
 *
 * Purpose:
 *   Updates the camera target and eye position in Scratchpad+0x1F0..0x204 based on
 *   Scratchpad+0x309 (camera/cutscene mode) and the active player's +0x3C8 flag,
 *   then copies those vectors to Scratchpad+0x210..0x22C. If Scratchpad+0x2F2 == 3,
 *   marks Scratchpad+0x2F0 = 0xFF and exits early.
 *
 * Arguments:
 *   None
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Scratchpad (0x1F800000):
 *     +0x1F0..+0x1F8 (3 x s32) working camera eye X/Y/Z (W/R)
 *     +0x1FC..+0x20C (4 x s32) working camera target X/Y/Z/+0x20C (W/R)
 *     +0x210..+0x22C (8 x s32) committed camera eye & target vectors (W)
 *     +0x2AD (u8)  active player slot (% 24) (R)
 *     +0x2F0 (u8)  set to 0xFF when Scratchpad+0x2F2 == 3 (W)
 *     +0x2F2 (u8)  camera state; == 3 triggers early exit, set to 3 in mode 2/3 (R/W)
 *     +0x2F4 (ptr) ball entity (+0x02 x, +0x06 z, +0x3A4 angle) (R)
 *     +0x309 (u8)  camera mode (1, 2, 3) (R)
 *   D_800FF748: player array base (stride 0x3E8 = 1000 bytes)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define P(p,o)  (*(unsigned char**)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800FF748[];

extern int  func_8001E694(unsigned int angle, int scale);
extern int  func_8001E5A4(unsigned int angle, int scale);
extern void func_80051A84(unsigned char *out);

void func_800975A0(void) {
    unsigned char *actor;
    unsigned char *ball;
    unsigned char mode;

    if (B(Scratchpad, 0x2F2) == 3) {
        B(Scratchpad, 0x2F0) = 0xFF;
        return;
    }

    actor = D_800FF748 + ((B(Scratchpad, 0x2AD) % 24U) * 0x3E8);
    mode = B(Scratchpad, 0x309);

    if (mode == 1) {
        if (S(actor, 0x3C8) != 0) {
            ball = P(Scratchpad, 0x2F4);
            SW(Scratchpad, 0x200) = -0x1C0;
            SW(Scratchpad, 0x1FC) = S(ball, 0x02) - func_8001E694(B(ball, 0x3A4), 0x800);

            ball = P(Scratchpad, 0x2F4);
            SW(Scratchpad, 0x204) = S(ball, 0x06) - func_8001E5A4(B(ball, 0x3A4), 0x800);

            SW(Scratchpad, 0x1F4) = -0x600;
            SW(Scratchpad, 0x1F0) = S(ball, 0x02) - func_8001E694(B(ball, 0x3A4), 0x2400);

            ball = P(Scratchpad, 0x2F4);
            SW(Scratchpad, 0x1F8) = S(ball, 0x06) - func_8001E5A4(B(ball, 0x3A4), 0x2400);
        } else {
            ball = P(Scratchpad, 0x2F4);
            SW(Scratchpad, 0x200) = -0x1C0;
            SW(Scratchpad, 0x1FC) = S(ball, 0x02);
            SW(Scratchpad, 0x204) = S(ball, 0x06);

            SW(Scratchpad, 0x1F4) = -0x500;
            SW(Scratchpad, 0x1F0) = S(ball, 0x02) - func_8001E694(B(ball, 0x3A4), 0x1C00);

            ball = P(Scratchpad, 0x2F4);
            SW(Scratchpad, 0x1F8) = S(ball, 0x06) - func_8001E5A4(B(ball, 0x3A4), 0x1C00);
        }
    } else if (mode == 2) {
        if (S(actor, 0x3C8) != 0) {
            SW(Scratchpad, 0x200) = -0x100;
            SW(Scratchpad, 0x1FC) = S(actor, 0x02);
            SW(Scratchpad, 0x204) = S(actor, 0x06);
            SW(Scratchpad, 0x1F4) = -0x700;
            SW(Scratchpad, 0x1F0) = S(actor, 0x02);
            SW(Scratchpad, 0x1F8) = S(actor, 0x06) - 0x2000;
        } else {
            B(Scratchpad, 0x2F2) = 3;
            func_80051A84(Scratchpad + 0x1F0);
        }
    } else if (mode == 3) {
        B(Scratchpad, 0x2F2) = 3;
        func_80051A84(Scratchpad + 0x1F0);
    }

    W(Scratchpad, 0x210) = W(Scratchpad, 0x1F0);
    W(Scratchpad, 0x214) = W(Scratchpad, 0x1F4);
    W(Scratchpad, 0x218) = W(Scratchpad, 0x1F8);
    W(Scratchpad, 0x21C) = W(Scratchpad, 0x1FC);
    W(Scratchpad, 0x220) = W(Scratchpad, 0x200);
    W(Scratchpad, 0x224) = W(Scratchpad, 0x204);
    W(Scratchpad, 0x228) = W(Scratchpad, 0x208);
    W(Scratchpad, 0x22C) = W(Scratchpad, 0x20C);
}
