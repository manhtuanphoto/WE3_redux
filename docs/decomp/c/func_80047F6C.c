/*
 * func_80047F6C
 * Purpose: Player ball-chasing and interception steering. Predicts intercept point via
 *          func_800484F8, slows/faces ball when already at the intercept point, or accelerates/
 *          delegates to func_80048D94 / func_80048AD8 when running toward the ball.
 * Arguments:
 *   actor - pointer to 1000-byte player actor struct
 *   arg1  - byte flag (non-zero allows sprint/run steering even if actor != ball owner)
 * Return value:
 *   void
 * Scratchpad reads/writes:
 *   R: 0x36/0x66 (s16 ball/target x/z), 0xF8/0xFC (s16 predicted x/z),
 *      0x2A9 (ball owner), 0x2B8 (s16), 0x2CF[side] (possession), 0x2F4 (ball actor ptr)
 *   W: 0xF2 (s16 = -128 / -0.512 m), 0xF8 (adjusted by +-128 / +-0.512 m)
 * Actor reads/writes:
 *   R: +0x02/+0x06 (x/z), +0x38D (actor id), +0x38E (anim id), +0x390 (anim frame),
 *      +0x398 (side), +0x399 (attack dir), +0x3A0 (speed), +0x3AA (body heading),
 *      +0x3B4 (max speed), +0x3B5 (accel divisor)
 *   W: +0x3A0 (speed), +0x3A4 (move heading), +0x3AA (body heading)
 * Constants:
 *   0x40 (64 raw = 0.256 m) arrival radius; 0x81 (129 raw = 0.516 m) close head-on radius;
 *   0x23E2 (9186 raw = 36.744 m) X threshold; 0x80 (128 raw = 0.512 m) X lead nudge;
 *   0x201 (513 raw = 2.052 m) sprint anim threshold;
 *   0x30 (48) default/min speed, 0x1E (30) min chase speed, 0x12 (18) decel per frame.
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define SCRATCH  ((unsigned char *)0x1F800000)

extern unsigned int func_800484F8(int x, int z, unsigned int speed, int arg3);
extern void func_80048AD8(unsigned char *actor, unsigned int ang_pred, unsigned int ang_alt, unsigned int dist);
extern void func_80048D94(unsigned char *actor, unsigned int ang_pred, unsigned int ang_alt, unsigned int dist);
extern int func_80054A68(unsigned char *actor, unsigned int ang);
extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);
extern unsigned int func_8005B7CC(unsigned int target, unsigned int cur, unsigned int step);
extern void func_8005BA30(unsigned char *actor, int anim, int frame);
extern void func_8005BA78(unsigned char *actor, int anim, int frame);
extern unsigned int func_800B1128(unsigned int val);

void func_80047F6C(unsigned char *actor, int arg1) {
    unsigned int max_spd;
    unsigned int pred_frames;
    unsigned char *ball;
    int fx;
    unsigned int ball_h;
    unsigned int abs_h;
    unsigned int ang_pred;
    unsigned int ang_alt;
    int dx;
    int dz;
    unsigned int dist;
    unsigned int diff;
    unsigned int owner;
    unsigned int accel;
    unsigned int spd;

    max_spd = 0x30;
    if (W(actor, 0x3A0) >= 0x31) {
        max_spd = W(actor, 0x3A0);
    }

    S(SCRATCH, 0xF2) = -0x80; /* -0.512 m */
    pred_frames = func_800484F8(S(actor, 2), S(actor, 6), max_spd & 0xFFFF, 1) & 0xFF;

    if (B(SCRATCH, 0x2A9) == 0) {
        ball = (unsigned char *)W(SCRATCH, 0x2F4);
        if (W(ball, 0x3A0) >= 0x5000 && S(SCRATCH, 0x2B8) == 0x7FFF) {
            fx = S(SCRATCH, 0xF8);
            if ((B(actor, 0x399) != 0 ? -fx : fx) < 0x23E2) { /* < 36.744 m */
                if (B(SCRATCH, 0x2CF + B(actor, 0x398)) == 0) {
                    ball = (unsigned char *)W(SCRATCH, 0x2F4);
                    ball_h = B(ball, 0x3A4);
                    abs_h = (ball_h < 0x80) ? ball_h : (unsigned int)(-(int)ball_h);
                    if (((abs_h - 0x10) & 0xFF) < 0x41) {
                        if (B(actor, 0x399) == 0) {
                            S(SCRATCH, 0xF8) = S(SCRATCH, 0xF8) - 0x80;
                        } else {
                            S(SCRATCH, 0xF8) = S(SCRATCH, 0xF8) + 0x80;
                        }
                    }
                }
            }
        }
    }

    ang_pred = func_8005B36C(S(actor, 2), S(actor, 6), S(SCRATCH, 0xF8), S(SCRATCH, 0xFC));
    ang_alt = func_8005B36C(S(actor, 2), S(actor, 6), S(SCRATCH, 0x36), S(SCRATCH, 0x66));

    dx = S(actor, 2) - S(SCRATCH, 0xF8);
    dz = S(actor, 6) - S(SCRATCH, 0xFC);
    dist = func_800B1128((unsigned int)(dx * dx + dz * dz));

    if (dist >= 0x40) { /* >= 0.256 m */
        ball = (unsigned char *)W(SCRATCH, 0x2F4);
        ball_h = B(ball, 0x3A4);
        diff = (ball_h - ang_pred) & 0xFF;
        if (diff < 0x81) {
            if (diff >= 0x78 && dist < 0x81) { /* < 0.516 m */
                goto stop_at_point;
            }
        } else {
            if (((ang_pred - ball_h) & 0xFF) >= 0x78 && dist < 0x81) {
                goto stop_at_point;
            }
        }
        goto steer_chase;
    }

stop_at_point:
    if (H(actor, 0x38E) != 1) {
        func_8005BA30(actor, 1, 0);
    }
    B(actor, 0x3AA) = func_8005B7CC(ang_alt & 0xFF, B(actor, 0x3AA), 8);
    spd = W(actor, 0x3A0);
    if (spd < 0x13) {
        W(actor, 0x3A0) = 0;
    } else {
        W(actor, 0x3A0) = spd - 0x12;
    }
    return;

steer_chase:
    if (func_80054A68(actor, ang_pred & 0xFF) != 0) {
        return;
    }

    if (H(actor, 0x38E) == 0x59) {
        func_8005BA30(actor, 4, B(actor, 0x390));
    }

    if (H(actor, 0x38E) == 4) {
        owner = B(SCRATCH, 0x2A9);
        if ((arg1 & 0xFF) != 0 || B(actor, 0x38D) == owner) {
            diff = (ang_pred - B(actor, 0x3AA)) & 0xFF;
            if (diff < 0x81) {
                if (diff >= 0x41) {
                    goto call_48D94;
                }
            } else {
                if (((B(actor, 0x3AA) - ang_pred) & 0xFF) >= 0x41) {
                    goto call_48D94;
                }
            }

            if ((dist / 48) + 2 < pred_frames) {
                goto call_48D94;
            }

            B(actor, 0x3AA) = func_8005B7CC(ang_pred & 0xFF, B(actor, 0x3AA), 4);
            accel = (unsigned int)(B(actor, 0x3B4) / (B(actor, 0x3B5) - 2));
            B(actor, 0x3A4) = ang_pred & 0xFF;
            spd = W(actor, 0x3A0) + accel;
            W(actor, 0x3A0) = spd;
            if (B(actor, 0x3B4) < spd) {
                W(actor, 0x3A0) = B(actor, 0x3B4);
            }
            if (W(actor, 0x3A0) < 0x1E) {
                W(actor, 0x3A0) = 0x1E;
            }
            return;

        call_48D94:
            func_80048D94(actor, ang_pred & 0xFF, ang_alt & 0xFF, dist & 0xFFFF);
            return;
        }
    }

    func_80048AD8(actor, ang_pred & 0xFF, ang_alt & 0xFF, dist & 0xFFFF);
    if ((arg1 & 0xFF) != 0 &&
        B(actor, 0x3AA) == (ang_alt & 0xFF) &&
        B(actor, 0x3A4) == (ang_pred & 0xFF) &&
        W(actor, 0x3A0) >= 0x30 &&
        (dist & 0xFFFF) >= 0x201) { /* >= 2.052 m */
        func_8005BA78(actor, 4, B(actor, 0x390));
    }
}
