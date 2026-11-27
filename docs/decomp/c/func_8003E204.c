/*
 * func_8003E204
 *
 * Purpose:
 *   Updates a player actor in the pressing / challenge / recovery substate
 *   machine (substates 0x00, 0x10, 0x11, 0x20, 0x30) and, when the ball is
 *   free, attempts a body/foot deflection or intercept via func_8009462C.
 *
 * Arguments:
 *   actor (unsigned char *): pointer to 1000-byte player actor struct.
 *
 * Return value:
 *   void.
 *
 * Scratchpad (0x1F800000) reads/writes:
 *   - 0x03C/0x06C (s16, R): ball predicted target x/z (used in substate 0x30).
 *   - 0x0F8/0x0FC (s16, R): predicted target x/z (used in substate 0x10).
 *   - 0x2A8 (u8, R): last-touch actor ID.
 *   - 0x2A9 (u8, R): current ball owner (0 = free).
 *   - 0x2AA (u8, R): previous ball owner.
 *   - 0x2B0 (u32, R): ball event code (1 = kick/pass check via func_8004E410).
 *   - 0x2F4 (unsigned char *, R): pointer to ball actor.
 *   - 0x31E (u8[2][2], R): marked/closest actor IDs indexed by attack_dir * 2.
 *   - 0x328 (u8, R): CPU difficulty/reaction level (indexes D_800C6D0C).
 *
 * Actor fields reads/writes:
 *   - +0x02/+0x06 (s16, R): position x/z.
 *   - +0x38D (u8, R): actor ID.
 *   - +0x390 (u8, R): animation frame.
 *   - +0x392 (u8, R): team slot (0..10).
 *   - +0x396 (u8, R/W): actor state (set to 1, 0xF, or 0x1B).
 *   - +0x397 (u8, R/W): actor substate (0x00, 0x10, 0x11, 0x20, 0x30).
 *   - +0x398 (u8, R): side (0/1).
 *   - +0x399 (u8, R): attack direction flag (0/1).
 *   - +0x39A (u8, R): CPU control flag.
 *   - +0x3A0 (u32, R/W): current speed.
 *   - +0x3A7 (u8, W): set to 1.
 *   - +0x3AA (u8, R/W): body heading (0..255 = 360 deg).
 *   - +0x3AC (u8, R): mirror/foot flag.
 *   - +0x3AE (u16, R/W): stamina/energy counter (decremented by 3).
 *   - +0x3C2 (s16, R): target actor ID.
 *   - +0x3C6 (s16, R/W): speed cap / momentum parameter.
 *   - +0x3CC (s16, R): deflection pitch override flag.
 *   - +0x3D0 (u8, R/W): action lock/cooldown timer (frames).
 *   - +0x3D1 (u8, W): cleared to 0 or set to 2.
 *
 * Ball actor (+0x2F4) reads:
 *   - +0x02/+0x06 (s16, R): ball x/z.
 *   - +0x3A0 (u32, R): ball speed.
 *   - +0x3A4 (u8, R): ball move heading.
 *
 * Magic numbers (250 raw = 1 m, 30 Hz):
 *   - 0x9000 (36864): 192^2 raw^2 (0.768 m distance hysteresis margin).
 *   - 0x10000 (65536): 256^2 raw^2 (1.024 m ball proximity threshold).
 *   - 0x19 (25): ~35.2 deg heading alignment tolerance.
 *   - 0x31 (49): ~68.9 deg heading alignment tolerance.
 *   - 0x24 (36): max speed cap in substate 0 (0.144 m/frame = 4.32 m/s).
 *   - -0x30 (-48): pitch parameter passed to func_8009462C when +0x3CC != 0.
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))

extern unsigned char D_800FF748[];
extern unsigned char D_800C6CF0[];
extern unsigned char D_800C6D0C[];
extern unsigned int D_8010C328[];

extern void func_8005BD1C(unsigned char *actor);
extern void func_8005BA30(unsigned char *actor, int anim, int frame);
extern void func_8004840C(unsigned char *actor);
extern int func_8005B36C(int x1, int z1, int x2, int z2);
extern int func_8005B7CC(int target, int cur, int step);
extern void func_8003EA64(unsigned char *actor, int mode);
extern int func_8001E6B8(int n);
extern void func_8005653C(unsigned char *actor, int mode);
extern void func_8005B864(unsigned char *actor, unsigned short speed);
extern int func_8004E410(unsigned char *actor);
extern int func_8004E5A8(int heading);
extern int func_8009462C(unsigned char *actor, int mode, int heading, int pitch, int lift);

void func_8003E204(unsigned char *actor)
{
    unsigned char *scratch = (unsigned char *)0x1F800000;
    unsigned char substate;

    B(actor, 0x3A7) = 1;
    substate = B(actor, 0x397);
    H(actor, 0x3AE) -= 3;

    switch (substate) {
    case 0:
    {
        func_8005BD1C(actor);
        B(actor, 0x3D1) = 0;

        if (B(actor, 0x390) == 0) {
            unsigned char actor_id;
            func_8005BA30(actor, 0x26, 0xF);
            actor_id = B(actor, 0x38D);
            if (actor_id == B(scratch, 0x2A8)) {
                B(actor, 0x396) = 1;
                B(actor, 0x397) = 0;
            } else if (actor_id == B(scratch, 0x31E + B(actor, 0x399) * 2)) {
                B(actor, 0x396) = 0x1B;
                B(actor, 0x397) = 1;
            } else {
                B(actor, 0x396) = 1;
                B(actor, 0x397) = 0;
            }
        }

        {
            unsigned int speed = W(actor, 0x3A0) + 9;
            if (speed >= 0x25) {
                speed = 0x24;
            }
            W(actor, 0x3A0) = speed;
        }
        break;
    }

    case 0x10:
    {
        int angle;
        int target_idx;
        unsigned char *target;
        unsigned char *ball;
        int dx, dz;
        int dist_self, dist_target;
        int diff;
        int closer_or_eq;

        func_8005BD1C(actor);
        func_8004840C(actor);
        angle = func_8005B36C(S(actor, 2), S(actor, 6), S(scratch, 0xF8), S(scratch, 0xFC));

        target_idx = S(actor, 0x3C2) % 24;
        target = D_800FF748 + target_idx * 1000;

        if (B(scratch, 0x2A9) != 0) {
            ball = (unsigned char *)W(scratch, 0x2F4);
            dx = S(actor, 2) - S(ball, 2);
            dz = S(actor, 6) - S(ball, 6);
            dist_self = dx * dx + dz * dz;

            dx = S(target, 2) - S(ball, 2);
            dz = S(target, 6) - S(ball, 6);
            dist_target = dx * dx + dz * dz;

            if (dist_self >= dist_target + 0x9000) {
                if (B(actor, 0x3AC) == 0) {
                    angle = (angle & 0xFF) + 6;
                } else {
                    angle = (angle & 0xFF) - 6;
                }
            }
        }

        diff = (angle - B(actor, 0x3AA)) & 0xFF;
        if (diff < 0x81 ? (diff < 0x19) : (((B(actor, 0x3AA) - angle) & 0xFF) < 0x19)) {
            int cap = S(actor, 0x3C6);
            unsigned int speed = W(actor, 0x3A0) + (cap / 4);
            if ((unsigned int)cap < speed) {
                speed = (unsigned int)cap;
            }
            W(actor, 0x3A0) = speed;
        } else {
            unsigned int speed = W(actor, 0x3A0);
            if (speed < 0x33) {
                W(actor, 0x3A0) = 0x22;
            } else {
                W(actor, 0x3A0) = speed - 0x10;
            }
        }

        ball = (unsigned char *)W(scratch, 0x2F4);
        dx = S(actor, 2) - S(ball, 2);
        dz = S(actor, 6) - S(ball, 6);
        dist_self = dx * dx + dz * dz;

        dx = S(target, 2) - S(ball, 2);
        dz = S(target, 6) - S(ball, 6);
        dist_target = dx * dx + dz * dz;

        closer_or_eq = (dist_target >= dist_self);
        if (closer_or_eq) {
            int cap;
            unsigned int target_speed;
            B(actor, 0x3AA) = func_8005B7CC(angle & 0xFF, B(actor, 0x3AA), 0xC);
            cap = S(actor, 0x3C6);
            target_speed = W(target, 0x3A0);
            if (target_speed >= (unsigned int)cap) {
                S(actor, 0x3C6) = (signed short)target_speed;
            }
        } else {
            if (B(actor, 0x390) < 5) {
                B(actor, 0x3AA) = func_8005B7CC(angle & 0xFF, B(actor, 0x3AA), 2);
            }
        }

        func_8003EA64(actor, 1);
        if (B(actor, 0x396) == 0x10 || B(actor, 0x390) != 0) {
            break;
        }

        func_8005BA30(actor, 0x26, 1);
        if (B(actor, 0x38D) == B(scratch, 0x2AA)) {
            B(actor, 0x396) = 0x1B;
            B(actor, 0x397) = 1;
            break;
        }

        if (closer_or_eq) {
            B(actor, 0x396) = 0xF;
            B(actor, 0x397) = 0;
            break;
        }

        B(actor, 0x3D0) = func_8001E6B8(5) + 4;
        B(actor, 0x3D1) = 2;
        B(actor, 0x397) = 0x11;
        func_8005653C(actor, 1);
        break;
    }

    case 0x11:
        func_8005BD1C(actor);
        if (B(actor, 0x3D0) != 0) {
            break;
        }
        if (B(actor, 0x39A) != 0) {
            unsigned char diff_lvl = B(scratch, 0x328);
            unsigned char *ball = (unsigned char *)W(scratch, 0x2F4);
            int dx = S(actor, 2) - S(ball, 2);
            int dz = S(actor, 6) - S(ball, 6);
            if (diff_lvl == 0 || dx * dx + dz * dz > 0x10000) {
                if (W(ball, 0x3A0) == 0) {
                    B(actor, 0x3D0) = 8;
                } else {
                    B(actor, 0x3D0) = D_800C6D0C[diff_lvl];
                }
            }
        }
        B(actor, 0x396) = 1;
        B(actor, 0x397) = 0;
        break;

    case 0x20:
    {
        unsigned char frame;
        func_8005BD1C(actor);
        func_8003EA64(actor, 2);

        frame = B(actor, 0x390);
        W(actor, 0x3A0) = (D_800C6CF0[frame] * S(actor, 0x3C6)) / 28;
        if (B(actor, 0x390) != 0) {
            break;
        }

        if (B(actor, 0x38D) == B(scratch, 0x2AA) && B(scratch, 0x2A9) == 0) {
            func_8005BA30(actor, 0x26, 0xF);
            if (B(actor, 0x38D) == B(scratch, 0x31E + B(actor, 0x399) * 2)) {
                B(actor, 0x396) = 0x1B;
                B(actor, 0x397) = 1;
            } else {
                B(actor, 0x396) = 1;
                B(actor, 0x397) = 0;
            }
        } else {
            int rnd;
            unsigned int stat_word;
            func_8005653C(actor, 0);
            func_8005BA30(actor, 1, 0);
            rnd = func_8001E6B8(5) + 0xC;
            stat_word = *(unsigned int *)((unsigned char *)D_8010C328 +
                                          B(actor, 0x392) * 12 + B(actor, 0x398) * 264);
            B(actor, 0x397) = 0x30;
            B(actor, 0x3D0) = rnd - (stat_word >> 28);
        }
        W(actor, 0x3A0) = 0;
        break;
    }

    case 0x30:
    {
        int dir = func_8005B36C(S(actor, 2), S(actor, 6), S(scratch, 0x3C), S(scratch, 0x6C));
        unsigned int speed;
        B(actor, 0x3AA) = func_8005B7CC(dir & 0xFF, B(actor, 0x3AA), 0x10);
        func_8005BD1C(actor);
        speed = W(actor, 0x3A0);
        if (speed < 0x13) {
            W(actor, 0x3A0) = 0;
        } else {
            W(actor, 0x3A0) = speed - 0x12;
        }

        if (B(actor, 0x3D0) == 0) {
            if (B(actor, 0x39A) != 0) {
                unsigned char *ball = (unsigned char *)W(scratch, 0x2F4);
                if (W(ball, 0x3A0) == 0) {
                    B(actor, 0x3D0) = 2;
                } else {
                    B(actor, 0x3D0) = D_800C6D0C[B(scratch, 0x328)];
                }
            }
            B(actor, 0x396) = 0;
            B(actor, 0x397) = 0;
        }
        break;
    }

    default:
        break;
    }

    func_8005B864(actor, H(actor, 0x3A0));

    if (B(scratch, 0x2A9) == 0) {
        int pitch_param;
        int defl_heading;
        int lift;

        if (S(actor, 0x3CC) == 0) {
            pitch_param = 0;
            if (W(scratch, 0x2B0) == 1) {
                defl_heading = func_8004E410(actor);
            } else {
                unsigned char *ball = (unsigned char *)W(scratch, 0x2F4);
                int ball_heading = B(ball, 0x3A4);
                int body_heading = B(actor, 0x3AA);
                int diff = (body_heading - ball_heading) & 0xFF;
                int aligned;

                if (func_8001E6B8(2) == 0) {
                    aligned = (diff < 0x81) ? (diff < 0x31) : (((ball_heading - body_heading) & 0xFF) < 0x31);
                } else {
                    aligned = 1;
                }

                if (aligned) {
                    unsigned char *ball2 = (unsigned char *)W(scratch, 0x2F4);
                    int cur_body = B(actor, 0x3AA);
                    int ball_dir = B(ball2, 0x3A4);
                    int opp_dir = cur_body + 0x80;
                    int rev_ball = (ball_dir + 0x80) & 0xFF;
                    int step = (ball_dir - opp_dir) & 0xFF;
                    if (step < 0x81) {
                        step = (unsigned int)step >> 1;
                    } else {
                        step = (unsigned int)((opp_dir - ball_dir) & 0xFF) >> 1;
                    }
                    defl_heading = func_8005B7CC(cur_body, rev_ball, step);
                } else {
                    unsigned char *ball2 = (unsigned char *)W(scratch, 0x2F4);
                    defl_heading = B(ball2, 0x3A4) - 0x80;
                }
            }
            defl_heading &= 0xFF;
        } else {
            unsigned char *ball = (unsigned char *)W(scratch, 0x2F4);
            pitch_param = -0x30;
            defl_heading = (B(ball, 0x3A4) - 0x80) & 0xFF;
        }

        lift = func_8004E5A8(defl_heading);
        func_8009462C(actor, 0xC, defl_heading, pitch_param, lift);
    }
}
