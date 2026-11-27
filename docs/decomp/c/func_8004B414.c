/*
 * func_8004B414
 *
 * Purpose:
 *   Simulates 12 future frames (0.4 s at 30 Hz) of ball physics using scratchpad
 *   working state (0x1F8000B0..0x1F8000BC) and writes the predicted ball X, Y, Z
 *   and horizontal speed for frames 0..11 into Scratchpad arrays. Handles
 *   stopped (0), airborne with gravity/spin/drag/bounce (1), ground roll (2),
 *   carried by owner (3), and resting check (4).
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Actor / global table fields read:
 *   ball+0x02/+0x04/+0x06 (s16) ball xyz,
 *   ball+0x0C (s32) ball vertical velocity vy,
 *   ball+0x396 (u8) ball state, ball+0x3A0 (u32) ball speed << 8,
 *   ball+0x3A4 (u8) ball heading, ball+0x3BC (u16) surface/bounce type,
 *   ball+0x3C8 (u16) curve/spin period, ball+0x3CA (u16) curve/spin direction,
 *   D_800FF748[(owner % 24) * 1000] + 0x3A0 (u32) owner speed (in state 3)
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x0FA (s16) predicted y (checked == -0x20 in state 4),
 *   +0x290 (u32) frame counter, +0x2A9 (u8) ball owner,
 *   +0x2F4 (u8*) pointer to ball actor
 *
 * Scratchpad fields written:
 *   +0x036[12] (s16) predicted ball x for frames 0..11,
 *   +0x04E[12] (s16) predicted ball y for frames 0..11,
 *   +0x066[12] (s16) predicted ball z for frames 0..11,
 *   +0x080[12] (u32) predicted ball speed << 8 for frames 0..11,
 *   +0x0B0..+0x0BC temporary ball state, heading, spin, speed, vy, surface
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   12 frames = 0.4 s horizon at 30 Hz,
 *   -0x20 = -32 (-0.128 m ball ground-contact height / radius),
 *   -0x44FF = -17663 (bounce threshold to clear surface flag),
 *   -0x5BF = -1471 (minimum upward rebound vy after bounce to stay airborne),
 *   Gravity per frame by surface 0..3: 0x2E0 (736), 0x2C0 (704), 0x320 (800), 0x338 (824),
 *   Air drag divisors: 14672900 (horizontal) and 14680064 (vertical),
 *   0x100 = 256 (ball speed << 8 stop threshold -> speed raw < 1)
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800FF748[];

extern int func_8001E5A4(unsigned int angle, int scale);
extern int func_8001E694(unsigned int angle, int scale);
extern unsigned int func_800B1128(unsigned int val);

void func_8004B414(void)
{
    unsigned char *ball = (unsigned char *)W(Scratchpad, 0x2F4);
    int x = S(ball, 0x02);
    int y = S(ball, 0x04);
    int z = S(ball, 0x06);
    int frame = 0;

    B(Scratchpad, 0x0B1) = B(ball, 0x3A4);
    B(Scratchpad, 0x0B2) = (unsigned char)H(ball, 0x3C8);
    B(Scratchpad, 0x0B3) = (unsigned char)H(ball, 0x3CA);
    W(Scratchpad, 0x0B4) = W(ball, 0x3A0);
    SW(Scratchpad, 0x0B8) = SW(ball, 0x0C);
    B(Scratchpad, 0x0B0) = B(ball, 0x396);
    B(Scratchpad, 0x0BC) = (unsigned char)H(ball, 0x3BC);

    for (frame = 0; frame < 12; frame++) {
        switch (B(Scratchpad, 0x0B0)) {
        case 0:
            W(Scratchpad, 0x0B4) = 0;
            break;

        case 4:
            if (SW(Scratchpad, 0x0B8) == 0 && S(Scratchpad, 0x0FA) == -0x20) {
                B(Scratchpad, 0x0B0) = 2;
            } else {
                B(Scratchpad, 0x0B0) = 1;
            }
            break;

        case 1: {
            unsigned int spinPeriod = B(Scratchpad, 0x0B2);
            int stepX;
            int stepZ;
            int vy;

            if (spinPeriod != 0 && ((frame + SW(Scratchpad, 0x290)) % (int)spinPeriod) == 0) {
                if (B(Scratchpad, 0x0B3) != 0) {
                    B(Scratchpad, 0x0B1) = B(Scratchpad, 0x0B1) + 1;
                } else {
                    B(Scratchpad, 0x0B1) = B(Scratchpad, 0x0B1) - 1;
                }
            }

            stepX = func_8001E694(B(Scratchpad, 0x0B1), SW(Scratchpad, 0x0B4)) >> 8;
            x += stepX;
            vy = SW(Scratchpad, 0x0B8) >> 8;
            y += vy;
            stepZ = func_8001E5A4(B(Scratchpad, 0x0B1), SW(Scratchpad, 0x0B4)) >> 8;
            z += stepZ;

            if ((signed short)y < -0x20) {
                switch (B(Scratchpad, 0x0BC)) {
                case 0:
                    SW(Scratchpad, 0x0B8) += 0x2E0;
                    break;
                case 1:
                    SW(Scratchpad, 0x0B8) += 0x2C0;
                    break;
                case 2:
                    SW(Scratchpad, 0x0B8) += 0x320;
                    break;
                case 3:
                    SW(Scratchpad, 0x0B8) += 0x338;
                    break;
                }
            } else {
                unsigned int spd = W(Scratchpad, 0x0B4);
                int vyPlus = SW(Scratchpad, 0x0B8) + 1;
                unsigned int absVy = (unsigned int)((vyPlus < 0) ? -vyPlus : vyPlus);
                int ratio = (int)(spd / absVy);
                unsigned int surface = B(Scratchpad, 0x0BC);

                if (ratio >= 9) {
                    ratio = 8;
                }

                switch (surface) {
                case 0: {
                    int curVy = SW(Scratchpad, 0x0B8);
                    W(Scratchpad, 0x0B4) = spd - ((spd * (unsigned int)(0x40 - ratio)) >> 8);
                    SW(Scratchpad, 0x0B8) = curVy - ((curVy * (0x60 - ratio)) / 256);
                    y = -0x20;
                    break;
                }
                case 1: {
                    int r4 = ratio << 2;
                    int curVy = SW(Scratchpad, 0x0B8);
                    W(Scratchpad, 0x0B4) = spd - ((spd * (unsigned int)(0x40 - r4)) >> 7);
                    SW(Scratchpad, 0x0B8) = curVy - ((curVy * (0x40 - r4)) / 240);
                    y = -0x20;
                    break;
                }
                case 2: {
                    int r4 = ratio << 2;
                    int curVy = SW(Scratchpad, 0x0B8);
                    B(Scratchpad, 0x0BC) = 0;
                    W(Scratchpad, 0x0B4) = spd - ((unsigned int)(spd * (0x40 - r4)) / 272U);
                    SW(Scratchpad, 0x0B8) = curVy - ((curVy * (0x60 - r4)) / 288);
                    y = -0x20;
                    break;
                }
                case 3: {
                    int r4 = ratio << 2;
                    int curVy = SW(Scratchpad, 0x0B8);
                    B(Scratchpad, 0x0BC) = 0;
                    W(Scratchpad, 0x0B4) = spd - ((unsigned int)(spd * (0x40 - r4)) / 288U);
                    SW(Scratchpad, 0x0B8) = curVy - ((curVy * (0x60 - r4)) / 304);
                    y = -0x20;
                    break;
                }
                default:
                    y = -0x20;
                    break;
                }

                SW(Scratchpad, 0x0B8) = -SW(Scratchpad, 0x0B8);
                if (B(Scratchpad, 0x0B2) != 0) {
                    B(Scratchpad, 0x0B2) = B(Scratchpad, 0x0B2) + 2;
                }
                if (SW(Scratchpad, 0x0B8) >= -0x44FF) {
                    B(Scratchpad, 0x0BC) = 0;
                }
                if (SW(Scratchpad, 0x0B8) >= -0x5BF) {
                    SW(Scratchpad, 0x0B8) = 0;
                    B(Scratchpad, 0x0B0) = 2;
                }
            }

            {
                int curSpd = SW(Scratchpad, 0x0B4);
                int curVy = SW(Scratchpad, 0x0B8);
                int totalSpd = (int)func_800B1128((unsigned int)(curSpd * curSpd + curVy * curVy));
                W(Scratchpad, 0x0B4) = (unsigned int)(curSpd - (int)((unsigned int)(totalSpd * curSpd) / 14672900U));
                SW(Scratchpad, 0x0B8) = curVy - ((totalSpd * curVy) / 14680064);
            }
            break;
        }

        case 2: {
            unsigned int spd;
            y = -0x20;
            x += func_8001E694(B(Scratchpad, 0x0B1), SW(Scratchpad, 0x0B4)) >> 8;
            z += func_8001E5A4(B(Scratchpad, 0x0B1), SW(Scratchpad, 0x0B4)) >> 8;
            spd = W(Scratchpad, 0x0B4);
            spd = spd - (spd >> 6);
            B(Scratchpad, 0x0B2) = 0;
            spd -= (unsigned int)((int)spd * (int)spd) / 14672900U;
            W(Scratchpad, 0x0B4) = spd;
            if (spd < 0x100) {
                B(Scratchpad, 0x0B0) = 0;
            }
            break;
        }

        case 3: {
            unsigned int ownerIdx = B(Scratchpad, 0x2A9);
            unsigned char *owner = D_800FF748 + (ownerIdx % 24) * 1000;
            unsigned char heading = B((unsigned char *)W(Scratchpad, 0x2F4), 0x3A4);
            y = -0x20;
            x += func_8001E694(heading, SW(owner, 0x3A0));
            z += func_8001E5A4(heading, SW(owner, 0x3A0));
            W(Scratchpad, 0x0B4) = W(owner, 0x3A0);
            break;
        }
        }

        W(Scratchpad, 0x080 + (frame << 2)) = W(Scratchpad, 0x0B4);
        S(Scratchpad, 0x036 + (frame << 1)) = (signed short)x;
        S(Scratchpad, 0x04E + (frame << 1)) = (signed short)y;
        S(Scratchpad, 0x066 + (frame << 1)) = (signed short)z;
    }
}
