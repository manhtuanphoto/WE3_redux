/*
 * func_800479B8 - Player ball-chase / interception movement and braking controller.
 *
 * Purpose:
 *   Steers a player actor toward the predicted target (scratch 0xF8/0xFC) while
 *   checking incoming loose-ball danger/pressure, deciding whether to brake/hold
 *   (anim 1), turn sharply with running anim 0x26, step via func_80048D94, or
 *   switch to anim 4 when closely aligned with a fast ball.
 *
 * Arguments:
 *   actor (unsigned char *): player actor struct (1000 bytes).
 *   flag (int): mode/chase flag (0 or 1; cleared to 0 if loose-ball check fails
 *               high-speed brake threshold).
 *
 * Return value:
 *   void.
 *
 * Scratchpad reads/writes:
 *   - 0x1F800036 (s16, R): predicted ball X (frame 3 slot).
 *   - 0x1F80004E (s16, R): predicted ball Y height (-0x100 = -1.024 m threshold).
 *   - 0x1F800066 (s16, R): predicted ball Z (frame 3 slot).
 *   - 0x1F8000F8 (s16, R): predicted target X.
 *   - 0x1F8000FC (s16, R): predicted target Z.
 *   - 0x1F80029A (u8, R): match phase (1 = open play).
 *   - 0x1F8002A9 (u8, R): current ball owner (0 = free).
 *   - 0x1F8002CF[2] (u8, R): team possession flags indexed by side.
 *   - 0x1F8002F4 (u8 *, R): pointer to ball actor.
 *   - 0x1F80031C (s16, R): ball timer (frames since last touch).
 *
 * Actor fields reads/writes:
 *   - actor: +0x02/+0x06 (s16 x/z, R), +0x38D (u8 id, R), +0x38E (u16 anim id, R),
 *            +0x390 (u8 anim frame, R), +0x398 (u8 side, R),
 *            +0x3A0 (u32 speed, R/W), +0x3A4 (u8 move heading, R/W),
 *            +0x3AA (u8 body heading, R/W), +0x3B4 (u8 max speed, R),
 *            +0x3B5 (u8 accel divisor, R).
 *   - ball:  +0x02/+0x06 (s16 x/z, R), +0x3A0 (u32 ball speed << 8, R),
 *            +0x3A4 (u8 ball move heading, R).
 *
 * Magic numbers (250 raw = 1 m, 30 Hz):
 *   - -0x100 (-256 raw = 1.024 m height) = ball height cutoff when free.
 *   - 0x7E / 0x11 (126 / 17 angle units = ~177 deg / ~24 deg) = angle thresholds.
 *   - 0x90000 = pressure metric threshold from func_800494C0.
 *   - 0x180 (384 raw = 1.536 m) + (ball_speed >> 5) = max chase distance for braking.
 *   - 0x6000 (ball speed << 8 -> 0x60 = 96 raw/frame = 11.52 m/s) and 0x141 (321 raw = 1.284 m).
 *   - 0x40000 (512^2 -> 512 raw = 2.048 m) = squared XZ distance to predicted target.
 *   - 0x30 / 0x22 / 0x1E (48 / 34 / 30 raw/frame) = speed caps/floors.
 *   - 0x10 (16 frames = 0.533 s) = ball timer threshold.
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))

extern int func_8005B36C(int x1, int z1, int x2, int z2);
extern void func_8005BA30(unsigned char *actor, int anim, int frame);
extern void func_8005BA78(unsigned char *actor, int anim, int frame);
extern int func_800B1128(int val);
extern unsigned int func_800494C0(unsigned char *actor);
extern int func_80054A68(unsigned char *actor, int heading);
extern int func_8005B7CC(int target, int cur, int step);
extern void func_80048D94(unsigned char *actor, int target_angle, int ball_angle, int dist);
extern void func_80054A1C(unsigned char *actor);

void func_800479B8(unsigned char *actor, int flag)
{
    unsigned char *scratch = (unsigned char *)0x1F800000;
    unsigned char *ball;
    int target_angle;
    int ball_angle;
    int body_diff;
    int dist;
    int dx, dz;
    unsigned int cur_target_dir;

    target_angle = func_8005B36C(S(actor, 2), S(actor, 6), S(scratch, 0xF8), S(scratch, 0xFC));
    {
        int raw_diff = target_angle - B(actor, 0x3AA);
        if ((raw_diff & 0xFF) < 0x81) {
            body_diff = raw_diff;
        } else {
            body_diff = B(actor, 0x3AA) - target_angle;
        }
    }

    ball = (unsigned char *)W(scratch, 0x2F4);
    ball_angle = func_8005B36C(S(actor, 2), S(actor, 6), S(ball, 2), S(ball, 6));

    if (H(actor, 0x38E) == 0x59) {
        func_8005BA30(actor, 4, B(actor, 0x390));
    }

    dx = S(actor, 2) - S(scratch, 0x36);
    dz = S(actor, 6) - S(scratch, 0x66);
    dist = func_800B1128(dx * dx + dz * dz);

    if (B(scratch, 0x2A9) == 0 && S(scratch, 0x4E) >= -0x100) {
        int ball_heading;
        int d;
        int check_pressure = 0;

        ball = (unsigned char *)W(scratch, 0x2F4);
        ball_heading = B(ball, 0x3A4);
        d = (ball_heading - ball_angle) & 0xFF;
        if ((unsigned int)d < 0x81U) {
            if (d >= 0x7E) {
                check_pressure = 1;
            }
        } else {
            if (((ball_angle - ball_heading) & 0xFF) >= 0x7E) {
                check_pressure = 1;
            }
        }

        if (check_pressure) {
            if (func_800494C0(actor) > 0x90000U) {
                unsigned int ball_speed;
                ball = (unsigned char *)W(scratch, 0x2F4);
                ball_speed = W(ball, 0x3A0);
                if ((unsigned int)dist <= (ball_speed >> 5) + 0x180U) {
                    if (ball_speed >= 0x6000U && (unsigned int)dist < 0x141U) {
                        if (H(actor, 0x38E) != 1) {
                            func_8005BA30(actor, 1, 0);
                        }
                        if (W(actor, 0x3A0) < 0x13U) {
                            W(actor, 0x3A0) = 0;
                        } else {
                            W(actor, 0x3A0) -= 0x12;
                        }
                        return;
                    }
                    flag = 0;
                }
            }
        }
    }

    cur_target_dir = (unsigned int)(target_angle & 0xFF);
    if (func_80054A68(actor, (int)cur_target_dir) != 0) {
        return;
    }

    if (H(actor, 0x38E) == 4) {
        if ((flag & 0xFF) != 0 || B(actor, 0x38D) == B(scratch, 0x2A9)) {
            if ((body_diff & 0xFF) < 0x21U) {
                int heading = func_8005B7CC((int)cur_target_dir, B(actor, 0x3AA), 4);
                int accel = (int)B(actor, 0x3B4) / (int)(B(actor, 0x3B5) - 2);
                unsigned int spd;

                B(actor, 0x3AA) = (unsigned char)heading;
                B(actor, 0x3A4) = (unsigned char)heading;
                spd = W(actor, 0x3A0) + (unsigned int)accel;
                W(actor, 0x3A0) = spd;
                if (spd > B(actor, 0x3B4)) {
                    W(actor, 0x3A0) = B(actor, 0x3B4);
                }
                if (W(actor, 0x3A0) < 0x1EU) {
                    W(actor, 0x3A0) = 0x1E;
                }
                return;
            }
        }
    }

    if (B(scratch, 0x29A) == 1) {
        int use_turn_anim = 0;
        int flag_byte = flag & 0xFF;

        if (flag_byte != 0 && W(actor, 0x3A0) >= 0x30U) {
            if (flag_byte != 1) {
                use_turn_anim = 1;
            } else {
                int d = (ball_angle - target_angle) & 0xFF;
                if ((unsigned int)d < 0x81U) {
                    if (d < 0x70) {
                        use_turn_anim = 1;
                    }
                } else {
                    if (((target_angle - ball_angle) & 0xFF) < 0x70) {
                        use_turn_anim = 1;
                    }
                }
            }
        }

        if (!use_turn_anim) {
            dx = S(actor, 2) - S(scratch, 0xF8);
            dz = S(actor, 6) - S(scratch, 0xFC);
            dist = func_800B1128(dx * dx + dz * dz);
            func_80048D94(actor, target_angle & 0xFF, ball_angle & 0xFF, dist & 0xFFFF);
            goto post_move;
        }
    }

    if (B(scratch, 0x2CF + B(actor, 0x398)) == 0) {
        func_80054A1C(actor);
    }
    func_8005BA78(actor, 0x26, B(actor, 0x390));
    B(actor, 0x3AA) = (unsigned char)func_8005B7CC((int)cur_target_dir, B(actor, 0x3AA), 0xC);
    B(actor, 0x3A4) = (unsigned char)func_8005B7CC((int)cur_target_dir, B(actor, 0x3A4), 0x18);
    if (B(actor, 0x3AA) == cur_target_dir) {
        W(actor, 0x3A0) += 0xC;
        if (W(actor, 0x3A0) >= 0x31U) {
            W(actor, 0x3A0) = 0x30;
        }
    } else {
        W(actor, 0x3A0) += 8;
        if (W(actor, 0x3A0) >= 0x23U) {
            W(actor, 0x3A0) = 0x22;
        }
    }

post_move:
    if ((flag & 0xFF) != 0 && W(actor, 0x3A0) == 0x30U) {
        unsigned int owner = B(scratch, 0x2A9);

        if (owner == 0) {
            if (B(scratch, 0x2CF + B(actor, 0x398)) != 0 && S(scratch, 0x31C) < 0x10) {
                return;
            }
        } else if (B(actor, 0x38D) != owner) {
            func_80054A1C(actor);
            func_8005BA78(actor, 4, B(actor, 0x390));
            return;
        }

        {
            int ball_heading;
            int d;
            int aligned = 0;

            ball = (unsigned char *)W(scratch, 0x2F4);
            ball_heading = B(ball, 0x3A4);
            d = (ball_heading - target_angle) & 0xFF;
            if ((unsigned int)d < 0x81U) {
                if (d < 0x11) {
                    aligned = 1;
                }
            } else {
                if (((target_angle - ball_heading) & 0xFF) < 0x11) {
                    aligned = 1;
                }
            }

            if (!aligned) {
                dx = S(actor, 2) - S(scratch, 0xF8);
                dz = S(actor, 6) - S(scratch, 0xFC);
                if (dx * dx + dz * dz <= 0x40000) {
                    ball = (unsigned char *)W(scratch, 0x2F4);
                    if ((W(ball, 0x3A0) >> 8) < 0x30U) {
                        return;
                    }
                }
            }
        }

        func_80054A1C(actor);
        func_8005BA78(actor, 4, B(actor, 0x390));
    }
}
