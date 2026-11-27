/*
 * func_800A3EC8
 *
 * Purpose:
 *   Computes an off-ball player's AI target (x, z) position by starting from
 *   the formation base position (func_8005A85C), applying possession- and
 *   role-dependent push-up / drop-back shifts based on ball depth and team
 *   mentality/line height (scratch 0x2FF[side]), pulling toward the ball via
 *   func_8005A76C (except defenders role < 5 out of possession), and clamping
 *   to pitch boundaries.
 *
 * Arguments:
 *   actor (unsigned char *): player's 1000-byte actor struct
 *   out_x (signed short *):  output target X coordinate
 *   out_z (signed short *):  output target Z coordinate
 *
 * Return value:
 *   void
 *
 * Scratchpad / Actor fields read or written:
 *   Scratch reads:  0x2CF[side] (team possession flag), 0x2F4 (ptr to ball),
 *                   0x2FF[side] (team line/mentality offset)
 *   Actor reads:    +0x391 (player role 1..25), +0x398 (side), +0x399 (attack dir)
 *   Ball reads:     +0x02 (ball x), +0x06 (ball z)
 *
 * Magic numbers / units (250 raw = 1 m):
 *   0xBC1  = 3009 raw (12.036 m), 0x1991 = 6545 raw (26.18 m),
 *   0x20E3 = 8419 raw (33.676 m), 0x23E3 = 9187 raw (36.748 m)
 *   0x32F0 = 13040 raw (52.16 m) max |x| pitch clamp
 *   0x20D0 = 8400 raw (33.60 m) max |z| pitch clamp
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define SCRATCH ((unsigned char*)0x1F800000)

void func_8005A85C(unsigned char *actor, signed short *tx, signed short *tz);
signed short func_8005A76C(int cur, int ball_coord, int mode);

void func_800A3EC8(unsigned char *actor, signed short *out_x, signed short *out_z)
{
    int dir_sign;
    signed short tx;
    signed short tz;
    unsigned char role;
    unsigned char *ball;
    int abs_bx;

    dir_sign = (B(actor, 0x399) != 0) ? -1 : 1;
    func_8005A85C(actor, &tx, &tz);

    if (B(SCRATCH, 0x2CF + B(actor, 0x398)) == 0) {
        role = B(actor, 0x391);
        switch (role) {
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
            ball = (unsigned char *)W(SCRATCH, 0x2F4);
            if ((B(actor, 0x399) == 0 && S(ball, 2) > 0) ||
                (B(actor, 0x399) == 1 && S(ball, 2) < 0)) {
                abs_bx = S(ball, 2);
                if (abs_bx < 0) {
                    abs_bx = -abs_bx;
                }
                if (abs_bx >= 0x20E3) {
                    tx = (signed short)(tx + (signed short)dir_sign * 9 * 256);
                } else if (abs_bx >= 0x1991) {
                    tx = (signed short)(tx + (signed short)dir_sign * 9 * 128);
                }
            }
            tx = (signed short)(tx + B(SCRATCH, 0x2FF + B(actor, 0x398)) * ((signed short)dir_sign * 512));
            break;

        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
            tx = (signed short)(tx + B(SCRATCH, 0x2FF + B(actor, 0x398)) * ((signed short)dir_sign * 5 * 128));
            /* fallthrough */
        case 14:
        case 15:
        case 16:
            ball = (unsigned char *)W(SCRATCH, 0x2F4);
            if ((B(actor, 0x399) == 0 && S(ball, 2) > 0) ||
                (B(actor, 0x399) == 1 && S(ball, 2) < 0)) {
                abs_bx = S(ball, 2);
                if (abs_bx < 0) {
                    abs_bx = -abs_bx;
                }
                if (abs_bx >= 0x20E3) {
                    tx = (signed short)(tx + (signed short)dir_sign * 3 * 512);
                } else if (abs_bx >= 0x1991) {
                    tx = (signed short)(tx + (signed short)dir_sign * 3 * 256);
                }
            }
            break;

        case 19:
        case 20:
            tx = (signed short)(tx + B(SCRATCH, 0x2FF + B(actor, 0x398)) * ((signed short)dir_sign * 3 * 256));
            ball = (unsigned char *)W(SCRATCH, 0x2F4);
            if ((B(actor, 0x399) == 0 && S(ball, 2) >= 0xBC1) ||
                (B(actor, 0x399) == 1 && S(ball, 2) < -0xBC0)) {
                abs_bx = S(ball, 2);
                if (abs_bx < 0) {
                    abs_bx = -abs_bx;
                }
                if (abs_bx >= 0x23E3) {
                    tx = (signed short)(tx + (signed short)dir_sign * 4096);
                } else if (abs_bx >= 0x1991) {
                    tx = (signed short)(tx + (signed short)dir_sign * 11 * 256);
                } else {
                    tx = (signed short)(tx + (signed short)dir_sign * 3 * 512);
                }
            }
            /* fallthrough */
        case 17:
        case 18:
        case 21:
        case 22:
        case 23:
        case 24:
        case 25:
            ball = (unsigned char *)W(SCRATCH, 0x2F4);
            if ((B(actor, 0x399) == 0 && S(ball, 2) > 0) ||
                (B(actor, 0x399) == 1 && S(ball, 2) < 0)) {
                abs_bx = S(ball, 2);
                if (abs_bx < 0) {
                    abs_bx = -abs_bx;
                }
                if (abs_bx >= 0x20E3) {
                    tx = (signed short)(tx + (signed short)dir_sign * 3 * 256);
                } else if (abs_bx >= 0x1991) {
                    tx = (signed short)(tx + (signed short)dir_sign * 3 * 128);
                }
            }
            break;

        default:
            break;
        }

        if (B(actor, 0x391) >= 5) {
            ball = (unsigned char *)W(SCRATCH, 0x2F4);
            tx = func_8005A76C(tx, S(ball, 2), 1);
            ball = (unsigned char *)W(SCRATCH, 0x2F4);
            tz = func_8005A76C(tz, S(ball, 6), 1);
        }
    } else {
        role = B(actor, 0x391);
        switch (role) {
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
            ball = (unsigned char *)W(SCRATCH, 0x2F4);
            if ((B(actor, 0x399) == 0 && S(ball, 2) < 0) ||
                (B(actor, 0x399) == 1 && S(ball, 2) > 0)) {
                abs_bx = S(ball, 2);
                if (abs_bx < 0) {
                    abs_bx = -abs_bx;
                }
                if (abs_bx >= 0x20E3) {
                    tx = (signed short)(tx - (signed short)dir_sign * 3 * 512);
                } else if (abs_bx >= 0x1991) {
                    tx = (signed short)(tx - (signed short)dir_sign * 3 * 256);
                }
            }
            break;

        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
            ball = (unsigned char *)W(SCRATCH, 0x2F4);
            if ((B(actor, 0x399) == 0 && S(ball, 2) < 0) ||
                (B(actor, 0x399) == 1 && S(ball, 2) > 0)) {
                abs_bx = S(ball, 2);
                if (abs_bx < 0) {
                    abs_bx = -abs_bx;
                }
                if (abs_bx >= 0x20E3) {
                    tx = (signed short)(tx - (signed short)dir_sign * 5 * 256);
                } else if (abs_bx >= 0x1991) {
                    tx = (signed short)(tx - (signed short)dir_sign * 5 * 128);
                }
            }
            /* fallthrough */
        case 17:
        case 18:
        case 19:
        case 20:
        case 21:
        case 22:
        case 23:
        case 24:
        case 25:
            ball = (unsigned char *)W(SCRATCH, 0x2F4);
            if ((B(actor, 0x399) == 0 && S(ball, 2) < 0) ||
                (B(actor, 0x399) == 1 && S(ball, 2) > 0)) {
                abs_bx = S(ball, 2);
                if (abs_bx < 0) {
                    abs_bx = -abs_bx;
                }
                if (abs_bx >= 0x20E3) {
                    tx = (signed short)(tx - (signed short)dir_sign * 1024);
                    tz = (signed short)(tz / 2);
                } else if (abs_bx >= 0x1991) {
                    tx = (signed short)(tx - (signed short)dir_sign * 512);
                    tz = (signed short)((tz * 3) / 4);
                }
            }
            break;

        default:
            break;
        }

        ball = (unsigned char *)W(SCRATCH, 0x2F4);
        tx = func_8005A76C(tx, S(ball, 2), 1);
        ball = (unsigned char *)W(SCRATCH, 0x2F4);
        tz = func_8005A76C(tz, S(ball, 6), 1);
    }

    if (tx > 0x32F0) {
        tx = 0x32F0;
    } else if (tx < -0x32F0) {
        tx = -0x32F0;
    }

    if (tz > 0x20D0) {
        tz = 0x20D0;
    } else if (tz < -0x20D0) {
        tz = -0x20D0;
    }

    *out_x = tx;
    *out_z = tz;
}
