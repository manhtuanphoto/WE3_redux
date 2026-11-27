/*
 * func_80023A78 - Player run/sprint-to-target state handler (substates 0..2).
 *
 * Purpose:
 *   Steers and moves a player actor toward their assigned target coordinates
 *   (+0x3BC, +0x3BE) across three substates:
 *   - Substate 0: Idle/orienting toward the ball (every 2nd frame based on actor ID
 *                 parity), facing goal line if ball is on ground behind player in open play
 *                 while team lacks possession; starts run (anim 2 -> substate 1) when
 *                 distance^2 > 0x40000 (2.05 m) and +0x3D0 == 0.
 *   - Substate 1: Jogging/strafing toward target (+0x3BC/+0x3BE). Clears target (+0x3D8 = 0,
 *                 substate = 0, speed >>= 1) if distance^2 < 0x1000 (0.26 m). Performs
 *                 hard brake/turn (anim 0xE) if reversing >= ~91 deg while moving fast;
 *                 transitions to sprint (anim 4 -> substate 2) if sprint input (0x22) is held
 *                 and speed >= 0x30.
 *   - Substate 2: Sprinting toward target (+0x3BC/+0x3BE) with acceleration
 *                 max_speed / (accel_div - 2), clamped to [0x1E, max_speed]. Clears target
 *                 if distance^2 < 0x1000, or drops back to substate 1 if sprint button released.
 *   - Post-switch: Integrates velocity (+0x08/+0x10) into position (+0x02/+0x06), updates
 *                  PS1 rotation (+0x26), and exits state if match phase != 1, freeze flag
 *                  (0x2FC) != 0, or actor owns/controls ball (0x2A9/0x240/0x241).
 *
 * Arguments:
 *   actor (u8*): player actor struct (1000 bytes).
 *
 * Return value:
 *   void.
 *
 * Scratchpad reads/writes:
 *   - 0x1F800022[2] (u16, R): per-side sprint/dash input bitmask (tested against actor +0x3D4).
 *   - 0x1F800240 / 0x1F800241 (u8, R): active controlled actor IDs.
 *   - 0x1F800290 (s32, R): global frame counter.
 *   - 0x1F80029A (u8, R): match phase (1 = open play).
 *   - 0x1F8002A9 (u8, R): ball owner actor ID.
 *   - 0x1F8002CF[2] (u8, R): per-side possession flag.
 *   - 0x1F8002F4 (u8*, R): pointer to ball actor.
 *   - 0x1F8002FC (u8, R): freeze/replay flag.
 *
 * Actor fields reads/writes:
 *   - actor: +0x02/+0x06 (s16 x/z, R/W), +0x08/+0x10 (s32 vx/vz, W), +0x18 (u32*, R),
 *            +0x1C (u32, W), +0x26 (u16 rotation, W), +0x38D (u8 id, R),
 *            +0x38E (u16 anim id, R), +0x390 (u8 anim frame, R/W), +0x396 (u8 state, W),
 *            +0x397 (u8 substate, R/W), +0x398 (u8 side, R), +0x399 (u8 attack dir, R),
 *            +0x39A (u8 role/GK flag, R), +0x3A0 (u32 speed, R/W),
 *            +0x3A4 (u8 move heading, R/W), +0x3A7 (u8, W=1), +0x3AA (u8 body heading, R/W),
 *            +0x3AB (u8 target heading, R/W), +0x3AC (u8 mirror/foot, R/W),
 *            +0x3B4 (u8 max speed, R), +0x3B5 (u8 accel divisor, R),
 *            +0x3BC/+0x3BE (s16 target x/z, R), +0x3D0 (u8 lock timer, R),
 *            +0x3D1 (u8, W=1), +0x3D4 (u16 input mask, R), +0x3D8 (s16, W=0).
 *   - ball:  +0x02/+0x04/+0x06 (s16 x/y/z, R).
 *
 * Magic numbers (250 raw = 1 m, 30 Hz):
 *   - 0x40000 (512^2) -> 2.05 m target distance threshold to begin running.
 *   - 0x1000 (64^2) -> 0.26 m arrival radius around target (+0x3BC/+0x3BE).
 *   - -0x19F (-415) -> 1.66 m ball height threshold.
 *   - Speeds: 0x30 (48) = 0.192 raw/frame (5.76 m/s sprint entry),
 *             0x22 (34) = turning jog floor, 0x1E (30) = sprint speed floor.
 *   - Angles: 0..255 = 360 deg (0x20 = 45 deg, 0x40 = 90 deg, 0x80 = 180 deg).
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))

extern void func_8005BA30(unsigned char *actor, int anim, int frame);
extern void func_8005BA78(unsigned char *actor, int anim, int frame);
extern int func_8005B36C(int x1, int z1, int x2, int z2);
extern int func_8005B7CC(int target, int cur, int step);
extern void func_80054A1C(unsigned char *actor);
extern void func_8005B908(unsigned char *actor);
extern int func_8001E694(int angle, int scale);
extern int func_8001E5A4(int angle, int scale);

void func_80023A78(unsigned char *actor)
{
    unsigned char *scratch = (unsigned char *)0x1F800000;
    unsigned char substate;
    int target_ang, ball_ang;
    int ang_diff;
    int dx, dz;
    unsigned int speed;

    substate = B(actor, 0x397);
    B(actor, 0x3A7) = 1;
    B(actor, 0x3D1) = 1;

    switch (substate) {
    case 0:
        if (H(actor, 0x38E) != 1) {
            func_8005BA30(actor, 1, 0);
        }
        W(actor, 0x3A0) = 0;

        if ((B(actor, 0x38D) & 1) == (unsigned int)(SW(scratch, 0x290) % 2)) {
            unsigned char *ball = (unsigned char *)W(scratch, 0x2F4);
            unsigned char cur_body;
            unsigned char want_body;
            int diff;

            ball_ang = func_8005B36C(S(actor, 2), S(actor, 6), S(ball, 2), S(ball, 6));
            ball = (unsigned char *)W(scratch, 0x2F4);

            if (S(ball, 4) >= -0x19F &&
                B(scratch, 0x29A) == 1 &&
                B(scratch, 0x2CF + B(actor, 0x398)) == 0) {
                int ball_behind;
                if (B(actor, 0x399) != 0) {
                    ball_behind = (-S(ball, 2) < -S(actor, 2));
                } else {
                    ball_behind = (S(ball, 2) < S(actor, 2));
                }
                if (ball_behind) {
                    B(actor, 0x3AB) = (unsigned char)func_8005B7CC(
                        (B(actor, 0x399) << 7) & 0x80,
                        ball_ang & 0xFF,
                        0x20
                    );
                } else {
                    B(actor, 0x3AB) = (unsigned char)ball_ang;
                }
            } else {
                B(actor, 0x3AB) = (unsigned char)ball_ang;
            }

            want_body = B(actor, 0x3AB);
            cur_body = B(actor, 0x3AA);
            diff = (int)want_body - (int)cur_body;
            diff = (diff + 0x100) - (((diff + 0x100) / 256) * 256);
            diff &= 0xFF;
            if (diff < 0x81) {
                if (diff < 0x11) {
                    B(actor, 0x3AA) = want_body;
                } else {
                    B(actor, 0x3AA) = (unsigned char)(cur_body + 0x20);
                }
            } else {
                if (0x100 - diff < 0x11) {
                    B(actor, 0x3AA) = want_body;
                } else {
                    B(actor, 0x3AA) = (unsigned char)(cur_body - 0x20);
                }
            }
        }

        dx = S(actor, 2) - S(actor, 0x3BC);
        dz = S(actor, 6) - S(actor, 0x3BE);
        B(actor, 0x3A4) = B(actor, 0x3AA);

        if (dx * dx + dz * dz > 0x40000 && B(actor, 0x3D0) == 0) {
            int ang;
            /* UNCLEAR: In the disassembly, `addu $a0, $s1, $zero` sits in the delay slot of
             * `jal func_8005B36C` (overwriting $a0 = S(actor, 2) with actor pointer $s1),
             * and `jal func_8005BA30` follows without reloading $a0. */
            ang = func_8005B36C((int)actor, S(actor, 6), S(actor, 0x3BC), S(actor, 0x3BE));
            B(actor, 0x3A4) = (unsigned char)ang;
            func_8005BA30(actor, 2, 0x10);
            B(actor, 0x397) = 1;
        }
        break;

    case 1:
    {
        unsigned char *ball;
        unsigned char frame;
        unsigned int *table;

        frame = (unsigned char)(B(actor, 0x390) + 1);
        B(actor, 0x390) = frame;
        table = (unsigned int *)W(actor, 0x18);
        /* UNCLEAR: Checks table[frame] != 0xB, then executes `sb $zero, 0x390($s1)` in
         * the delay slot unconditionally, resetting +0x390 to 0 in both branches. */
        (void)table[frame];
        B(actor, 0x390) = 0;

        dx = S(actor, 2) - S(actor, 0x3BC);
        dz = S(actor, 6) - S(actor, 0x3BE);
        W(actor, 0x1C) = table[B(actor, 0x390)];

        if (dx * dx + dz * dz < 0x1000) {
            S(actor, 0x3D8) = 0;
            B(actor, 0x397) = 0;
            W(actor, 0x3A0) = W(actor, 0x3A0) >> 1;
            break;
        }

        target_ang = func_8005B36C(S(actor, 2), S(actor, 6), S(actor, 0x3BC), S(actor, 0x3BE));
        B(actor, 0x3AB) = (unsigned char)target_ang;
        ball = (unsigned char *)W(scratch, 0x2F4);
        ball_ang = func_8005B36C(S(actor, 2), S(actor, 6), S(ball, 2), S(ball, 6));

        if (W(actor, 0x3A0) >= 0x20 || H(actor, 0x38E) == 0xE) {
            int move_diff = (B(actor, 0x3A4) - target_ang) & 0xFF;
            int rev = (move_diff < 0x81) ? (move_diff >= 0x41) : (((target_ang - B(actor, 0x3A4)) & 0xFF) >= 0x41);
            if (rev) {
                if (H(actor, 0x38E) != 0xE) {
                    func_80054A1C(actor);
                    B(actor, 0x3AC) = (unsigned char)(((ball_ang - target_ang) & 0xFF) < 0x81);
                    func_8005BA30(actor, 0xE, 2);
                }
                B(actor, 0x3AA) = (unsigned char)func_8005B7CC(ball_ang & 0xFF, B(actor, 0x3AA), 0x28);
                speed = W(actor, 0x3A0);
                W(actor, 0x3A0) = (speed < 9) ? 0 : (speed - 8);
                if (W(actor, 0x3A0) != 0) {
                    func_8005B908(actor);
                    return;
                }
                func_8005BA78(actor, 2, B(actor, 0x390));
                B(actor, 0x3A4) = (unsigned char)target_ang;
            }
        }

        ang_diff = (ball_ang - target_ang) & 0xFF;
        if (ang_diff >= 0x81) {
            ang_diff = target_ang - ball_ang;
        }
        if (H(actor, 0x38E) == 2 && (ang_diff & 0xFF) >= 0x10) {
            ang_diff -= 8;
        }
        if (H(actor, 0x38E) == 0x1B && (ang_diff & 0xFF) < 0x51) {
            ang_diff += 8;
        }

        if ((ang_diff & 0xFF) < 0x21) {
            func_8005BA78(actor, 2, B(actor, 0x390));
            B(actor, 0x3AA) = (unsigned char)func_8005B7CC(target_ang & 0xFF, B(actor, 0x3AA), 0xC);
            B(actor, 0x3A4) = (unsigned char)func_8005B7CC(target_ang & 0xFF, B(actor, 0x3A4), 0x18);
        } else {
            int body_diff = (target_ang - B(actor, 0x3AA)) & 0xFF;
            int body_close = (body_diff < 0x81)
                ? (body_diff < 0x20)
                : (((B(actor, 0x3AA) - target_ang) & 0xFF) < 0x20);

            if (H(actor, 0x38E) == 0x1B || body_close) {
                int strafe_ang;
                if ((ang_diff & 0xFF) < 0x79) {
                    B(actor, 0x3AC) = (unsigned char)(((target_ang - ball_ang) & 0xFF) < 0x81);
                }
                B(actor, 0x3A4) = (unsigned char)func_8005B7CC(target_ang & 0xFF, B(actor, 0x3A4), 0x18);
                strafe_ang = (B(actor, 0x3AC) != 0)
                    ? ((target_ang & 0xFF) - 0x28)
                    : ((target_ang & 0xFF) + 0x28);
                target_ang = strafe_ang;
                B(actor, 0x3AA) = (unsigned char)func_8005B7CC(target_ang & 0xFF, B(actor, 0x3AA), 0xC);
                if (H(actor, 0x38E) != 0x1B) {
                    func_8005BA78(actor, 0x1B, B(actor, 0x390));
                }
            } else {
                func_8005BA78(actor, 2, B(actor, 0x390));
                B(actor, 0x3AA) = (unsigned char)func_8005B7CC(target_ang & 0xFF, B(actor, 0x3AA), 0xC);
                B(actor, 0x3A4) = (unsigned char)func_8005B7CC(target_ang & 0xFF, B(actor, 0x3A4), 0x18);
            }
        }

        if (B(actor, 0x3AA) == (target_ang & 0xFF)) {
            speed = W(actor, 0x3A0) + 0xC;
            if (speed >= 0x31) {
                speed = 0x30;
            }
            W(actor, 0x3A0) = speed;
        } else {
            speed = W(actor, 0x3A0);
            W(actor, 0x3A0) = (speed >= 0x33) ? (speed - 0x10) : 0x22;
        }

        if ((H(scratch, 0x22 + B(actor, 0x398) * 2) & H(actor, 0x3D4)) != 0 &&
            W(actor, 0x3A0) >= 0x30) {
            func_8005BA78(actor, 4, B(actor, 0x390));
            B(actor, 0x397) = 2;
        }
        break;
    }

    case 2:
    {
        unsigned char frame;
        unsigned int *table;
        int d;

        frame = (unsigned char)(B(actor, 0x390) + 1);
        B(actor, 0x390) = frame;
        table = (unsigned int *)W(actor, 0x18);
        /* UNCLEAR: Checks table[frame] != 0xB, then executes `sb $zero, 0x390($s1)` in
         * the delay slot unconditionally, resetting +0x390 to 0 in both branches. */
        (void)table[frame];
        B(actor, 0x390) = 0;

        W(actor, 0x1C) = table[B(actor, 0x390)];
        target_ang = func_8005B36C(S(actor, 2), S(actor, 6), S(actor, 0x3BC), S(actor, 0x3BE));

        d = (target_ang - B(actor, 0x3AA)) & 0xFF;
        if (d >= 0x81) {
            d = B(actor, 0x3AA) - target_ang;
        }
        if ((d & 0xFF) < 0x21) {
            int accel;
            unsigned int max_spd;
            B(actor, 0x3AA) = (unsigned char)func_8005B7CC(target_ang & 0xFF, B(actor, 0x3AA), 4);
            B(actor, 0x3A4) = (unsigned char)func_8005B7CC(target_ang & 0xFF, B(actor, 0x3A4), 4);
            accel = (int)B(actor, 0x3B4) / ((int)B(actor, 0x3B5) - 2);
            speed = W(actor, 0x3A0) + (unsigned int)accel;
            max_spd = B(actor, 0x3B4);
            if (speed > max_spd) {
                speed = max_spd;
            }
            if (speed < 0x1E) {
                speed = 0x1E;
            }
            W(actor, 0x3A0) = speed;
        } else {
            func_8005BA78(actor, 2, B(actor, 0x390));
            B(actor, 0x3AA) = (unsigned char)func_8005B7CC(target_ang & 0xFF, B(actor, 0x3AA), 0xC);
            B(actor, 0x3A4) = (unsigned char)func_8005B7CC(target_ang & 0xFF, B(actor, 0x3A4), 0x18);
        }

        dx = S(actor, 2) - S(actor, 0x3BC);
        dz = S(actor, 6) - S(actor, 0x3BE);
        if (dx * dx + dz * dz < 0x1000) {
            S(actor, 0x3D8) = 0;
            B(actor, 0x397) = 0;
        } else if ((H(scratch, 0x22 + B(actor, 0x398) * 2) & H(actor, 0x3D4)) == 0) {
            func_8005BA78(actor, 2, B(actor, 0x390));
            S(actor, 0x3D8) = 0;
            B(actor, 0x397) = 1;
        }
        break;
    }
    }

    SW(actor, 8) = func_8001E694(B(actor, 0x3A4), W(actor, 0x3A0));
    SW(actor, 0x10) = func_8001E5A4(B(actor, 0x3A4), W(actor, 0x3A0));
    {
        int rot = 0x1C0 - (int)B(actor, 0x3AA);
        rot = rot - (rot / 256) * 256;
        H(actor, 0x26) = (unsigned short)(rot << 4);
    }
    S(actor, 2) = (signed short)(S(actor, 2) + SW(actor, 8));
    S(actor, 6) = (signed short)(S(actor, 6) + SW(actor, 0x10));

    if (B(scratch, 0x29A) != 1 || B(scratch, 0x2FC) != 0) {
        B(actor, 0x396) = 1;
        B(actor, 0x397) = 5;
    } else {
        unsigned char id = B(actor, 0x38D);
        if (id == B(scratch, 0x2A9) ||
            (B(actor, 0x39A) != 2 &&
             (id == B(scratch, 0x240) || id == B(scratch, 0x241)))) {
            B(actor, 0x396) = (H(actor, 0x38E) == 1) ? 0 : 1;
            B(actor, 0x397) = 0;
        }
    }
}
