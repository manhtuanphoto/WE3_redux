/*
 * func_80039A5C
 *
 * Purpose:
 *   Goalkeeper action/save state selector. Evaluates whether the GK can intercept
 *   or dive for the predicted ball position, choosing between dive/catch/punch
 *   states (0x0D, 0x1E..0x21, 0x25, 0x29, 0x2A, or func_80035944) based on box
 *   bounds, ball angle sector relative to GK, ball heading relative to goal,
 *   and predicted ball height.
 *
 * Arguments:
 *   actor (unsigned char *) - goalkeeper actor struct (1000 bytes)
 *
 * Return value:
 *   u8 (unsigned int masked to 0xFF) - next GK state (+0x396) or current state if no change
 *
 * Actor fields read:
 *   +0x02 (s16) x position
 *   +0x06 (s16) z position
 *   +0x396 (u8) current state
 *   +0x398 (u8) side (0/1)
 *   +0x399 (u8) attack direction flag
 *   +0x3D6 (u16) GK eligibility/zone mask
 *   Ball actor (+0x2F4): +0x02 (s16) x, +0x06 (s16) z, +0x3A4 (u8) move heading
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x12[side*2] (u16) zone/status bitmask tested against actor+0x3D6
 *   +0x46 (s16) predicted ball x
 *   +0x5E (s16) predicted ball y (height, negative = up)
 *   +0x76 (s16) predicted ball z
 *   +0x2F4 (u8 *) pointer to ball actor
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x1990 (6544 raw = 26.176 m) x threshold for GK box check
 *   0x16F5 (5877 raw = 23.508 m) |z| bound
 *   0x3320 (13088 raw = 52.352 m) goal line x
 *   -0x160 (-352 raw = 1.408 m up) high-ball height threshold
 *   -0xEF (-239 raw = 0.956 m up) mid-ball height threshold
 *   -0xA0 (-160 raw = 0.64 m up) low-ball height threshold
 *   0x281 (641 raw = 2.564 m) |dx| reach threshold
 *   0x31 (49 angle units = 68.9 deg), 0x21 (33 angle units = 46.4 deg) heading tolerances
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned int func_80052BF4(void);
extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);
extern unsigned int func_80039DD0(unsigned char *actor);
extern unsigned int func_80035944(unsigned char *actor);
extern unsigned int func_8003BC84(unsigned char *actor);
extern unsigned int func_8003F2B8(unsigned char *actor);
extern unsigned int func_8003A4A8(unsigned char *actor);
extern unsigned int func_8003ACA8(unsigned char *actor);
extern unsigned int func_8003B430(unsigned char *actor);

unsigned int func_80039A5C(unsigned char *actor)
{
    unsigned short pred_x;
    unsigned short pred_y;
    unsigned short pred_z;
    short x;
    int check_x;
    short z;
    int abs_z;
    unsigned int angle;
    unsigned int sector;
    unsigned char *ball;
    int goal_x;
    unsigned int goal_angle;
    unsigned int next_state;

    if ((func_80052BF4() & 0xFFU) == 0) {
        return B(actor, 0x396);
    }

    pred_x = H(Scratchpad, 0x46);
    pred_y = H(Scratchpad, 0x5E);
    pred_z = H(Scratchpad, 0x76);

    x = S(actor, 0x02);
    check_x = (B(actor, 0x399) != 0) ? -x : x;
    if (check_x >= 0x1990) {
        z = S(actor, 0x06);
        abs_z = (z < 0) ? -z : z;
        if (abs_z < 0x16F5) {
            angle = func_8005B36C(S(actor, 0x02), z, (short)pred_x, (short)pred_z) & 0xFFU;
            sector = angle >> 4;
            if (B(actor, 0x399) != 0) {
                int tmp = (int)(sector & 0xFFU) + 8;
                int adj = (tmp >= 0) ? tmp : ((int)(sector & 0xFFU) + 0x17);
                sector = tmp - (adj & 0x30);
            }

            ball = *(unsigned char **)(Scratchpad + 0x2F4);
            goal_x = (B(actor, 0x399) != 0) ? -0x3320 : 0x3320;
            goal_angle = func_8005B36C(S(ball, 0x02), S(ball, 0x06), goal_x, 0);

            switch (sector & 0xFFU) {
            case 7:
            case 8: {
                unsigned int ball_heading = B(*(unsigned char **)(Scratchpad + 0x2F4), 0x3A4);
                unsigned int diff = (goal_angle - ball_heading) & 0xFFU;
                int aligned;
                if (diff < 0x81U) {
                    aligned = ((int)diff < 0x31);
                } else {
                    aligned = ((int)((ball_heading - goal_angle) & 0xFFU) < 0x31);
                }
                if (!aligned) {
                    if ((func_80039DD0(actor) & 0xFFU) != 0) {
                        next_state = 0x20;
                        return next_state & 0xFFU;
                    }
                }
                if ((short)pred_y < -0x160) {
                    next_state = func_80035944(actor);
                    return next_state & 0xFFU;
                }
                next_state = B(actor, 0x396);
                return next_state & 0xFFU;
            }
            case 3:
            case 4:
            case 5:
            case 6:
            case 9:
            case 10:
            case 11:
            case 12: {
                unsigned int ball_heading;
                unsigned int diff;
                int aligned;
                if ((short)pred_y < -0x160) {
                    next_state = func_80035944(actor);
                    return next_state & 0xFFU;
                }
                ball_heading = B(*(unsigned char **)(Scratchpad + 0x2F4), 0x3A4);
                diff = (goal_angle - ball_heading) & 0xFFU;
                if (diff < 0x81U) {
                    aligned = ((int)diff < 0x21);
                } else {
                    aligned = ((int)((ball_heading - goal_angle) & 0xFFU) < 0x21);
                }
                if (aligned) {
                    next_state = B(actor, 0x396);
                    return next_state & 0xFFU;
                }
                if ((func_8003BC84(actor) & 0xFFU) != 0) {
                    next_state = 0x1E;
                    return next_state & 0xFFU;
                }
                if (func_8003F2B8(actor) != 0) {
                    next_state = 0x0D;
                    return next_state & 0xFFU;
                }
                if ((func_8003A4A8(actor) & 0xFFU) != 0) {
                    next_state = 0x1F;
                    return next_state & 0xFFU;
                }
                next_state = B(actor, 0x396);
                return next_state & 0xFFU;
            }
            case 0:
            case 1:
            case 2:
            case 13:
            case 14:
            case 15: {
                short s_pred_y = (short)pred_y;
                int dx;
                if (s_pred_y < -0x160) {
                    next_state = func_80035944(actor);
                    return next_state & 0xFFU;
                }
                if ((func_8003BC84(actor) & 0xFFU) != 0) {
                    next_state = 0x1E;
                    return next_state & 0xFFU;
                }
                dx = S(actor, 0x02) - (short)pred_x;
                if (dx < 0) {
                    dx = -dx;
                }
                if ((dx >= 0x281 && (int)s_pred_y < -0xA0) || (int)s_pred_y < -0xEF) {
                    if ((func_8003B430(actor) & 0xFFU) != 0) {
                        next_state = 0x21;
                        return next_state & 0xFFU;
                    }
                } else {
                    if ((func_8003ACA8(actor) & 0xFFU) != 0) {
                        next_state = 0x29;
                        return next_state & 0xFFU;
                    }
                }
                if (func_8003F2B8(actor) != 0) {
                    next_state = 0x0D;
                    return next_state & 0xFFU;
                }
                next_state = B(actor, 0x396);
                return next_state & 0xFFU;
            }
            default:
                /* UNCLEAR: in asm, sector >= 16 jumps to .L80039DA0 returning uninitialized $s0 & 0xFF;
                 * unreachable in practice because sector is (u8 >> 4) or rotated into 0..15. */
                return B(actor, 0x396);
            }
        }
    }

    if ((H(Scratchpad, 0x12 + B(actor, 0x398) * 2) & H(actor, 0x3D6)) == 0) {
        return B(actor, 0x396);
    }
    if ((short)pred_y < -0x160) {
        return 0x25;
    }
    return 0x2A;
}
