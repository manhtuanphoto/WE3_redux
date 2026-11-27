/*
 * func_8006B044
 *
 * Purpose:
 *   Adjusts the predicted/target x, z coordinates in Scratchpad (+0xF8, +0xFC)
 *   for an attacking/off-ball AI player:
 *   1) Blends Scratchpad+0xF8 toward ball.x (3:1), then if Scratchpad+0xA != 0
 *      applies further blending/forward offset based on Scratchpad+0x2FF[side]
 *      (0: 3:1 blend toward ball.x, 1: (2*x + ball.x + dir*1024)/3,
 *       2: (x + ball.x + dir*2048)/2).
 *   2) When the ball is in the player's own half relative to attack direction,
 *      if actor id matches D_8010C1D8[0xC], shifts +0xF8 to ball.x - dir*2816
 *      and adjusts +0xFC via func_8005A76C(z, ball.z, 3).
 *   3) Otherwise, when abs(ball.x) >= 0x665 (6.548 m), adjusts +0xFC/+0xF8
 *      to push out laterally (to +-19.408 m + z/10, or fixed +-19.408 m on
 *      matching role/formation conditions in D_8010C1D8[0xD+side]).
 *   4) If D_800FF748+0xABDC+side != 0, shifts +0xF8 by -dir*1024, and clamps
 *      +0xF8 to [-0x23E2, 0x23E2] (+-36.744 m).
 *
 * Arguments:
 *   actor - pointer to 1000-byte actor struct
 *
 * Return value:
 *   None (void)
 *
 * Actor / global fields read:
 *   +0x38D (u8) actor id, +0x391 (u8) role/slot, +0x398 (u8) side (0/1),
 *   +0x399 (u8) attack direction flag
 *   D_8010C1D8 + 0x0C (u8) designated support actor id
 *   D_8010C1D8 + 0x0D + side (u8) per-side tactical state (2 or 3 checked)
 *   D_800FF748 + 0xABDC + side (u8) per-side pullback flag
 *
 * Actor / global fields written:
 *   None
 *
 * Scratchpad (0x1F800000) fields read / written:
 *   +0x00A (u8) phase/timer flag
 *   +0x0F8 (s16) target x (read & written)
 *   +0x0FC (s16) target z (read & written)
 *   +0x2F4 (ptr) ball actor (+0x002 s16 x, +0x006 s16 z)
 *   +0x2FF[2] (u8) per-side mode (0, 1, 2)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   1024 (0x400) = 4.096 m, 2048 (0x800) = 8.192 m, 2816 (0xB00) = 11.264 m
 *   0x300 (768) = 3.072 m (with /4 -> +192 = +0.768 m bias in blend)
 *   0x665 (1637) = 6.548 m midfield x threshold
 *   0x12F4 (4852) = 19.408 m lateral width target
 *   0x23E2 (9186) = 36.744 m x clamp
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned char D_8010C1D8[];

extern int func_8005A76C(int targetZ, int ballZ, int step);

void func_8006B044(unsigned char *actor)
{
    int dirSign = (B(actor, 0x399) != 0) ? -1 : 1;
    unsigned char *ball = (unsigned char *)W(Scratchpad, 0x2F4);
    int x = (S(Scratchpad, 0xF8) * 3 + S(ball, 2)) / 4;
    int atkDir;

    S(Scratchpad, 0xF8) = (signed short)x;

    if (B(Scratchpad, 0xA) != 0) {
        unsigned char mode = B(Scratchpad, 0x2FF + B(actor, 0x398));
        if (mode == 0) {
            S(Scratchpad, 0xF8) = (signed short)((((signed short)x) * 3 + S(ball, 2)) / 4);
        } else if (mode == 1) {
            S(Scratchpad, 0xF8) = (signed short)((((signed short)x) * 2 + S(ball, 2) + dirSign * 1024) / 3);
        } else if (mode == 2) {
            S(Scratchpad, 0xF8) = (signed short)((((signed short)x) + S(ball, 2) + dirSign * 2048) / 2);
        }
    }

    atkDir = B(actor, 0x399);
    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    if ((atkDir == 0 && S(ball, 2) < 0) || (atkDir == 1 && S(ball, 2) > 0)) {
        if (B(actor, 0x38D) == B(D_8010C1D8, 0xC)) {
            S(Scratchpad, 0xF8) = (signed short)(H(ball, 2) - dirSign * 2816);
            S(Scratchpad, 0xFC) = (signed short)func_8005A76C(S(Scratchpad, 0xFC), S(ball, 6), 3);
        }
    } else {
        int absBallX = S(ball, 2);
        if (absBallX < 0) {
            absBallX = -absBallX;
        }
        if (absBallX >= 0x665) {
            int zSign = (S(Scratchpad, 0xFC) >= 0) ? 1 : -1;
            int bSign = (S(ball, 6) >= 0) ? 1 : -1;
            if (zSign == bSign) {
                int curZ = S(Scratchpad, 0xFC);
                int magZ = (curZ >= 0) ? curZ : -curZ;
                int newZ = magZ / 10 + 0x12F4;
                if (curZ < 0) {
                    newZ = -newZ;
                }
                S(Scratchpad, 0xFC) = (signed short)newZ;
                ball = (unsigned char *)W(Scratchpad, 0x2F4);
                S(Scratchpad, 0xF8) = (signed short)((S(Scratchpad, 0xF8) * 3 + S(ball, 2) + 0x300) / 4);
            } else {
                unsigned char tact = B(D_8010C1D8, 0xD + B(actor, 0x398));
                unsigned char role = B(actor, 0x391);
                if ((tact == 2 && (role == 0x13 || role == 0x11)) ||
                    (tact == 3 && (role == 0x14 || role == 0x12))) {
                    ball = (unsigned char *)W(Scratchpad, 0x2F4);
                    S(Scratchpad, 0xF8) = (signed short)((S(Scratchpad, 0xF8) * 3 + S(ball, 2) + 0x300) / 4);
                    S(Scratchpad, 0xFC) = (signed short)(dirSign * 0x12F4);
                }
            }

            if (B(D_800FF748, 0xABDC + B(actor, 0x398)) != 0) {
                S(Scratchpad, 0xF8) = (signed short)(H(Scratchpad, 0xF8) - dirSign * 1024);
            }
        }
    }

    {
        int clamped = S(Scratchpad, 0xF8);
        int absClamped = (clamped >= 0) ? clamped : -clamped;
        if (absClamped >= 0x23E3) {
            S(Scratchpad, 0xF8) = (clamped >= 0) ? 0x23E2 : -0x23E2;
        }
    }
}
