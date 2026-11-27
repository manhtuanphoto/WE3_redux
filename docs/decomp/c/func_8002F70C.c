/*
 * func_8002F70C
 *
 * Purpose:
 *   Player ball-chase / interception / trap-preparation state machine (runs when
 *   substate actor+0x397 is in 0..3 or 16..22). Handles approaching the predicted
 *   ball/target point, matching ball speed and heading, transitioning into ball
 *   control (state 1, 3, 4, or 5), and updating off-ball marking/pressure scratch
 *   fields.
 *
 * Arguments:
 *   actor (a0) - pointer to 1000-byte player actor struct
 *
 * Return value:
 *   void
 *
 * Scratchpad (0x1F800000) fields read/written:
 *   0x012[side] (u16, R): input/action mask (checked when 0x3CC == 4)
 *   0x022[side] (u16, R): input/action mask (pass/sprint/control override)
 *   0x02E[side] (u16, R): input/action mask (dummy/let-ball-run or control flag)
 *   0x036, 0x066 (s16, R): target/goal or cursor x, z
 *   0x038, 0x068 (s16, R): secondary target x, z
 *   0x03E, 0x06E (s16, R): pass/aim target x, z
 *   0x04E (s16, R): predicted ball height y (negative = up; < -116 raw = > 0.464 m)
 *   0x0F8, 0x0FC (s16, R/W): predicted/intercept target x, z
 *   0x0FE, 0x100 (s16, R): ball/intercept target x, z
 *   0x240, 0x241 (u8, R): actor IDs of designated chasers/markers
 *   0x28C (u8, R): team/tactic mode or target actor index
 *   0x2A9 (u8, R): current ball owner (0 = free ball)
 *   0x2AA (u8, R): previous ball owner
 *   0x2B0 (u32, W): ball event code (cleared to 0 on trap transition)
 *   0x2F4 (u32/ptr, R): pointer to ball actor
 *   0x31C (s16, R): ball timer (frames since last touch; 30 frames/s)
 *   0x31E[dir], 0x31F[dir] (u8, R/W): nearest/chasing actor ID per attack dir
 *   0x328 (u8, R): ball/possession flag
 *
 * Actor fields read/written:
 *   +0x002, +0x006 (s16, R/W): x, z position (250 raw = 1 m)
 *   +0x38D (u8, R): actor ID
 *   +0x38E (u16, R): current animation ID
 *   +0x390 (u8, R): animation frame
 *   +0x392 (u8, R): team slot (0..10)
 *   +0x396 (u8, W): actor state
 *   +0x397 (u8, R/W): actor substate (switch index 0..22)
 *   +0x398 (u8, R): side (0/1)
 *   +0x399 (u8, R): attack direction flag (0/1 -> heading 0/128)
 *   +0x39A (u8, R): CPU/AI control flag
 *   +0x3A0 (u32, R/W): speed
 *   +0x3A4 (u8, W): move heading (0..255 = 360 deg)
 *   +0x3A7 (u8, W): active/update flag (set to 1)
 *   +0x3AA (u8, R/W): body heading
 *   +0x3AB (u8, R/W): target heading toward intercept point
 *   +0x3AC (u8, W): mirror/foot flag
 *   +0x3AE (u16, R/W): countdown timer (decremented by 1 or 2 per frame)
 *   +0x3B0 (u8, W): ball/control timer (cleared to 0; ball+0x3B0 set to 16)
 *   +0x3B1 (u8, W): flag cleared to 0
 *   +0x3B3 (u8, R): sprint/run speed cap
 *   +0x3B4 (u8, R): max speed
 *   +0x3B5 (u8, R): acceleration divisor
 *   +0x3C2 (u16, W): target/contact flags
 *   +0x3C4 (u16, W): action/submode (set to 4)
 *   +0x3C6 (u16, R/W): charge/power counter (incremented by 2)
 *   +0x3C8 (u16, W): chaser match flag (1 if actor ID == 0x240 or 0x241)
 *   +0x3CA (s16, R): follow-up action mode (1 -> state 5 transition)
 *   +0x3CC (s16, R/W): target state / ball arrival timer (0xFF = default, 4 = trap)
 *   +0x3D0 (u8, R/W): delay frames before acceleration phase
 *   +0x3D1 (u8, R/W): chase mode flag (0, 1, 2, or 5)
 *   +0x3D4 (u16, R): input button mask
 *
 * Magic numbers & unit conversions:
 *   0x23FFF (147455) sq dist -> sqrt ~384 raw = ~1.54 m
 *   0x40 (64) dist -> 0.256 m
 *   0xFFFF (65535) sq dist -> sqrt 256 raw = 1.024 m
 *   0x90000 (589824) sq dist -> sqrt 768 raw = 3.072 m
 *   0x8FFFFF (9437183) sq dist -> sqrt ~3072 raw = ~12.29 m
 *   0x801 (2049) dist -> 8.196 m
 *   -0x74 (-116) height -> 0.464 m above ground
 *   0x3320 (13088) / -0x3320 (-13088) x -> +/- 52.352 m (goal line)
 *   0x10 (16) angle -> 22.5 deg; 0x20 (32) -> 45 deg; 0x40 (64) -> 90 deg;
 *   0x50 (80) -> 112.5 deg; 0x30 (48) -> 67.5 deg; 0x8 (8) -> 11.25 deg; 0x4 -> 5.625 deg
 *   Timers: 8/9 frames (~0.27-0.30 s), ball speed 0x2000 (8192 = 32 << 8),
 *   0x2800 (10240 = 40 << 8)
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))

#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned int D_8010C330[];
extern unsigned char D_8010C32B[];
extern unsigned char D_800C6B74[];

extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);
extern void func_8005BA78(unsigned char *actor, unsigned int anim, unsigned int frame);
extern void func_8005BD1C(unsigned char *actor);
extern void func_80045504(unsigned char *actor, unsigned int arg);
extern unsigned int func_800B1128(unsigned int val);
extern void func_8005BA30(unsigned char *actor, unsigned int anim, unsigned int frame);
extern unsigned int func_8004A07C(unsigned char *actor);
extern unsigned int func_80049398(unsigned char *actor, unsigned int heading, unsigned int arg2);
extern unsigned int func_8005B7CC(unsigned int target, unsigned int cur, unsigned int step);
extern unsigned int func_800494C0(unsigned char *actor);
extern void func_8002C28C(unsigned char *actor);
extern void func_80048FC4(unsigned char *actor, unsigned int ang1, unsigned int ang2, unsigned int dist);
extern void func_800479B8(unsigned char *actor, unsigned int arg);
extern void func_80054A1C(unsigned char *actor);
extern void func_80048D94(unsigned char *actor, unsigned int ang1, unsigned int ang2, unsigned int dist);
extern void func_8004840C(unsigned char *actor);
extern unsigned int func_8001E6B8(unsigned int n);
extern void func_8003172C(unsigned char *actor, unsigned int ang);
extern void func_800484B8(int x, int z, unsigned int max_spd);
extern void func_800512B8(unsigned char *actor, unsigned int arg);
extern void func_8002A83C(unsigned char *actor, unsigned int arg);
extern void func_8005B908(unsigned char *actor);

static unsigned int abs_angle_diff(unsigned int a, unsigned int b)
{
    unsigned int d = (a - b) & 0xFF;
    if (d >= 0x81) {
        d = (b - a) & 0xFF;
    }
    return d;
}

void func_8002F70C(unsigned char *actor)
{
    unsigned int substate;
    unsigned char *ball;
    unsigned int angle_target;
    unsigned int angle_misc;
    unsigned int desired_heading;
    unsigned int dist;
    unsigned int dist_sq;
    int dx;
    int dz;
    int ball_dx_fe;
    int ball_dz_fe;
    int ball_dx_f8;
    int ball_dz_f8;
    unsigned int ball_dist_fe_sq;
    unsigned int ball_dist_f8_sq;
    unsigned int speed;
    unsigned int calc_speed;
    int within_range;
    unsigned int row;
    unsigned int prev_id;
    unsigned int side_off;
    int next_state;

    B(actor, 0x3A7) = 1;
    substate = B(actor, 0x397);

    if (substate < 0x17) {
        switch (substate) {
        case 0:
            if (B(actor, 0x38D) == B(Scratchpad, 0x240) ||
                B(actor, 0x38D) == B(Scratchpad, 0x241)) {
                H(actor, 0x3C8) = 1;
            } else {
                H(actor, 0x3C8) = 0;
            }
            H(actor, 0x3CC) = 0xFF;
            H(actor, 0x3C2) = B(Scratchpad, 0x2AA);

            dx = S(actor, 0x2) - S(Scratchpad, 0xFE);
            dz = S(actor, 0x6) - S(Scratchpad, 0x100);
            dist_sq = (unsigned int)(dx * dx + dz * dz);
            angle_target = func_8005B36C(S(actor, 0x2), S(actor, 0x6), S(Scratchpad, 0xFE), S(Scratchpad, 0x100));
            B(actor, 0x3AB) = (unsigned char)angle_target;
            B(actor, 0x3D1) = 5;

            /* > ~1.54 m from target or target angle within 22.5 deg of attack dir */
            if (dist_sq > 0x23FFFU ||
                abs_angle_diff((unsigned int)B(actor, 0x399) << 7, B(actor, 0x3AB)) < 0x11U) {
                func_8005BA78(actor, 0x26, B(actor, 0x390));
                B(actor, 0x397) = 2;
                break;
            }
            B(actor, 0x397) = (unsigned char)(B(actor, 0x397) + 1);
            /* fallthrough to case 1 */

        case 1:
            H(actor, 0x3AE) = (unsigned short)(H(actor, 0x3AE) - 1);
            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            angle_target = func_8005B36C(S(ball, 0x2), S(ball, 0x6), S(Scratchpad, 0xFE), S(Scratchpad, 0x100));
            ball = (unsigned char *)W(Scratchpad, 0x2F4);

            if (abs_angle_diff(angle_target, B(ball, 0x3A4)) >= 0x41U ||
                W(ball, 0x3A0) < 0x2000U) {
                func_8005BD1C(actor);
                B(actor, 0x397) = 3;
                func_80045504(actor, 7);
                break;
            }

            dx = S(actor, 0x2) - S(Scratchpad, 0xFE);
            dz = S(actor, 0x6) - S(Scratchpad, 0x100);
            dist = func_800B1128((unsigned int)(dx * dx + dz * dz));

            if (dist < 0x40U) {
                /* Snap halfway toward intercept point (< 0.256 m) */
                S(actor, 0x2) = (signed short)((S(actor, 0x2) + S(Scratchpad, 0xFE)) >> 1);
                S(actor, 0x6) = (signed short)((S(actor, 0x6) + S(Scratchpad, 0x100)) >> 1);

                angle_misc = func_8005B36C(S(actor, 0x2), S(actor, 0x6), S(Scratchpad, 0x3E), S(Scratchpad, 0x6E));
                ball = (unsigned char *)W(Scratchpad, 0x2F4);
                if (abs_angle_diff(angle_misc, B(ball, 0x3A4)) < 0x41U) {
                    if (B(actor, 0x3D1) == 1) {
                        if (B(Scratchpad, 0x28C) == 0) {
                            if (H(actor, 0x38E) != 0x16) {
                                func_8005BA30(actor, 0x16, 0);
                            }
                            row = (unsigned int)B(actor, 0x392) * 12U + (unsigned int)B(actor, 0x398) * 264U;
                            B(actor, 0x3AC) = (unsigned char)((W((unsigned char *)D_8010C330, row) >> 4) & 1U);
                        } else if (H(actor, 0x38E) != 1) {
                            func_8005BA30(actor, 1, 0);
                        }
                    } else if (B(actor, 0x3D1) == 0) {
                        if (H(actor, 0x38E) != 0xE) {
                            func_8005BA30(actor, 0xE, 0);
                        }
                    } else if (H(actor, 0x38E) != 1) {
                        func_8005BA30(actor, 1, 0);
                    }
                } else if (H(actor, 0x38E) != 1) {
                    func_8005BA30(actor, 1, 0);
                }

                angle_target = func_8005B36C(S(actor, 0x2), S(actor, 0x6), S(Scratchpad, 0x36), S(Scratchpad, 0x66));
                if (S(actor, 0x3CA) == 1) {
                    unsigned int h = func_8005B7CC(((unsigned int)B(actor, 0x399) << 7) & 0x80U, angle_target & 0xFFU, 0x10);
                    unsigned int body_h = func_8005B7CC(h & 0xFFU, B(actor, 0x3AA), 8);
                    B(actor, 0x3AA) = (unsigned char)body_h;

                    dx = S(actor, 0x2) - S(Scratchpad, 0x36);
                    dz = S(actor, 0x6) - S(Scratchpad, 0x66);
                    if ((dx * dx + dz * dz) <= 0xFFFF) {
                        H(actor, 0x3C4) = 4;
                        B(actor, 0x3A4) = (unsigned char)body_h;
                        B(actor, 0x3B0) = 0;
                        B(actor, 0x396) = 5;
                        B(actor, 0x397) = 0;
                        func_8002C28C(actor);
                    }
                    B(actor, 0x3D1) = 2;
                } else {
                    ball = (unsigned char *)W(Scratchpad, 0x2F4);
                    dx = S(actor, 0x2) - S(ball, 0x2);
                    dz = S(actor, 0x6) - S(ball, 0x6);
                    dist = func_800B1128((unsigned int)(dx * dx + dz * dz));

                    if (S(Scratchpad, 0x4E) < -0x74) {
                        ball = (unsigned char *)W(Scratchpad, 0x2F4);
                        desired_heading = (unsigned int)((int)B(ball, 0x3A4) - 0x80);
                        B(actor, 0x3AA) = (unsigned char)func_8005B7CC(desired_heading & 0xFFU, B(actor, 0x3AA), 0x20);
                    } else {
                        if (B(actor, 0x39A) == 0 && B(Scratchpad, 0x28C) != 0) {
                            desired_heading = func_8004A07C(actor);
                        } else {
                            desired_heading = func_80049398(actor, ((unsigned int)B(actor, 0x399) << 7) & 0x80U, 0x90000);
                        }
                        ball = (unsigned char *)W(Scratchpad, 0x2F4);
                        if ((W(ball, 0x3A0) >> 5) < dist + 0x100U) {
                            desired_heading = func_8005B7CC(desired_heading & 0xFFU, (B(ball, 0x3A4) + 0x80U) & 0xFFU, 0x20);
                        }
                        B(actor, 0x3AA) = (unsigned char)func_8005B7CC(desired_heading & 0xFFU, B(actor, 0x3AA), 0x10);
                    }
                    B(actor, 0x3A4) = B(actor, 0x3AA);

                    if (S(Scratchpad, 0x31C) >= 9 &&
                        abs_angle_diff(desired_heading, B(actor, 0x3AA)) < 0x21U) {
                        ball = (unsigned char *)W(Scratchpad, 0x2F4);
                        if ((W(ball, 0x3A0) >> 5) + 0x100U < dist) {
                            dist_sq = dist * dist;
                            if (B(actor, 0x39A) == 0) {
                                if (B(Scratchpad, 0x28C) != 0) {
                                    unsigned int tgt_ang = B(D_800C6B74, B(Scratchpad, 0x28C));
                                    if (abs_angle_diff(tgt_ang, angle_target) < 0x31U) {
                                        B(actor, 0x397) = 3;
                                    }
                                }
                            } else {
                                unsigned int opp_dir = ((unsigned int)(B(actor, 0x399) ^ 1) << 7);
                                unsigned int adiff = abs_angle_diff(B(ball, 0x3A4), opp_dir) << 1;
                                unsigned int pressure = func_800494C0(actor);
                                unsigned int threshold = adiff * adiff + 0x24000U + pressure;

                                if (B(Scratchpad, 0x328) != 0) {
                                    if (W(ball, 0x3A0) < 0x2801U ||
                                        (threshold < dist_sq && dist_sq <= 0x8FFFFFU)) {
                                        B(actor, 0x397) = 3;
                                    }
                                }
                            }
                        }
                    }

                    if (H(Scratchpad, 0x2E + (unsigned int)B(actor, 0x398) * 2U) & H(actor, 0x3D4)) {
                        B(actor, 0x3D1) = 0;
                        ball = (unsigned char *)W(Scratchpad, 0x2F4);
                        B(ball, 0x3B0) = 0x10;
                    } else {
                        B(actor, 0x3D1) = 1;
                    }
                }

                speed = W(actor, 0x3A0);
                if (speed < 0x13U) {
                    W(actor, 0x3A0) = 0;
                } else {
                    W(actor, 0x3A0) = speed - 0x12U;
                }
            } else {
                angle_target = func_8005B36C(S(actor, 0x2), S(actor, 0x6), S(Scratchpad, 0xFE), S(Scratchpad, 0x100));
                angle_misc = func_8005B36C(S(actor, 0x2), S(actor, 0x6), S(Scratchpad, 0x36), S(Scratchpad, 0x66));
                func_80048FC4(actor, angle_target & 0xFFU, angle_misc & 0xFFU, dist & 0xFFFFU);
                B(actor, 0x3D1) = 1;
            }

            func_8005BD1C(actor);
            func_80045504(actor, 7);
            break;

        case 2:
            func_8005BD1C(actor);
            H(actor, 0x3AE) = (unsigned short)(H(actor, 0x3AE) - 1);
            angle_target = func_8005B36C(S(actor, 0x2), S(actor, 0x6), S(Scratchpad, 0xFE), S(Scratchpad, 0x100));
            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            desired_heading = func_8005B36C(S(actor, 0x2), S(actor, 0x6), S(ball, 0x2), S(ball, 0x6));

            dx = S(actor, 0x2) - S(Scratchpad, 0xFE);
            dz = S(actor, 0x6) - S(Scratchpad, 0x100);
            dist = func_800B1128((unsigned int)(dx * dx + dz * dz));

            if (dist < 0x40U) {
                speed = W(actor, 0x3A0);
                if (speed < 0x13U) {
                    W(actor, 0x3A0) = 0;
                } else {
                    W(actor, 0x3A0) = speed - 0x12U;
                }
                B(actor, 0x397) = 1;
            } else {
                ball = (unsigned char *)W(Scratchpad, 0x2F4);
                calc_speed = dist / (unsigned int)(S(ball, 0x3CC) + 1);
                if (H(actor, 0x38E) == 4) {
                    calc_speed += 8;
                }

                within_range = 0;
                if ((H(Scratchpad, 0x22 + (unsigned int)B(actor, 0x398) * 2U) & H(actor, 0x3D4)) != 0 &&
                    (int)calc_speed >= 0x31) {
                    within_range = 0;
                } else if (dist >= 0x801U) {
                    within_range = 0;
                } else {
                    within_range = ((int)calc_speed < 0x35);
                }

                if (within_range) {
                    func_80048D94(actor, angle_target & 0xFFU, desired_heading & 0xFFU, dist & 0xFFFFU);
                    if (W(actor, 0x3A0) == 0x30U) {
                        if ((int)calc_speed < 0x28) {
                            W(actor, 0x3A0) = 0x28;
                        } else if (!within_range) {
                            W(actor, 0x3A0) = 0x34;
                        } else {
                            W(actor, 0x3A0) = calc_speed;
                        }
                    }
                } else {
                    S(Scratchpad, 0xF8) = S(Scratchpad, 0xFE);
                    S(Scratchpad, 0xFC) = S(Scratchpad, 0x100);
                    func_800479B8(actor, 1);
                    if (W(actor, 0x3A0) >= 0x30U && H(actor, 0x38E) != 4) {
                        func_80054A1C(actor);
                        func_8005BA78(actor, 4, B(actor, 0x390));
                    }
                }
            }

            if (H(Scratchpad, 0x2E + (unsigned int)B(actor, 0x398) * 2U) & H(actor, 0x3D4)) {
                B(actor, 0x3D1) = 0;
                ball = (unsigned char *)W(Scratchpad, 0x2F4);
                B(ball, 0x3B0) = 0x10;
            } else {
                B(actor, 0x3D1) = 1;
            }
            func_8004840C(actor);

            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            ball_dx_fe = S(ball, 0x2) - S(Scratchpad, 0xFE);
            ball_dz_fe = S(ball, 0x6) - S(Scratchpad, 0x100);
            ball_dist_fe_sq = (unsigned int)(ball_dx_fe * ball_dx_fe + ball_dz_fe * ball_dz_fe);
            ball_dx_f8 = S(ball, 0x2) - S(Scratchpad, 0xF8);
            ball_dz_f8 = S(ball, 0x6) - S(Scratchpad, 0xFC);
            ball_dist_f8_sq = (unsigned int)(ball_dx_f8 * ball_dx_f8 + ball_dz_f8 * ball_dz_f8);

            if ((int)ball_dist_f8_sq >= (int)ball_dist_fe_sq) {
                func_8005BD1C(actor);
                B(actor, 0x397) = 3;
            }
            func_80045504(actor, 7);
            break;

        case 3:
            H(actor, 0x3AE) = (unsigned short)(H(actor, 0x3AE) - 1);
            if (B(actor, 0x3D1) != 0) {
                B(actor, 0x3D1) = 1;
            }
            func_8005BD1C(actor);
            func_8004840C(actor);

            angle_target = func_8005B36C(S(actor, 0x2), S(actor, 0x6), S(Scratchpad, 0xF8), S(Scratchpad, 0xFC));
            desired_heading = func_8005B36C(S(actor, 0x2), S(actor, 0x6), S(Scratchpad, 0x36), S(Scratchpad, 0x66));

            if (S(Scratchpad, 0x31C) >= 8 &&
                ((H(Scratchpad, 0x22 + (unsigned int)B(actor, 0x398) * 2U) & H(actor, 0x3D4)) != 0 ||
                 abs_angle_diff(B((unsigned char *)W(Scratchpad, 0x2F4), 0x3A4), desired_heading) < 0x51U)) {
                func_800479B8(actor, 1);
            } else {
                dx = S(actor, 0x2) - S(Scratchpad, 0xF8);
                dz = S(actor, 0x6) - S(Scratchpad, 0xFC);
                dist = func_800B1128((unsigned int)(dx * dx + dz * dz));
                func_80048D94(actor, angle_target & 0xFFU, desired_heading & 0xFFU, dist & 0xFFFFU);
                if (W(actor, 0x3A0) == 0x30U) {
                    W(actor, 0x3A0) = 0x36;
                }
            }

            if (abs_angle_diff(desired_heading, angle_target) < 5U && B(actor, 0x3D1) == 0) {
                W(Scratchpad, 0x2B0) = 0;
                B(actor, 0x396) = 1;
                B(actor, 0x397) = 0;
                break;
            }
            func_80045504(actor, 7);
            break;

        case 16:
            func_8005BD1C(actor);
            func_8005BA78(actor, 0x26, B(actor, 0x390));
            H(actor, 0x3AE) = (unsigned short)(H(actor, 0x3AE) - 2);

            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            if (abs_angle_diff((unsigned int)B(actor, 0x399) << 7, B(ball, 0x3A4)) < 0x11U) {
                ball = (unsigned char *)W(Scratchpad, 0x2F4);
                H(ball, 0x3CC) = (unsigned short)(H(ball, 0x3CC) + 2);
            }

            H(actor, 0x3CC) = 0xFF;
            B(actor, 0x3D1) = 5;
            H(actor, 0x3CA) = 2;
            B(actor, 0x3AB) = (unsigned char)func_8005B36C(S(actor, 0x2), S(actor, 0x6), S(Scratchpad, 0xFE), S(Scratchpad, 0x100));

            if (B(actor, 0x38D) == B(Scratchpad, 0x240) ||
                B(actor, 0x38D) == B(Scratchpad, 0x241)) {
                H(actor, 0x3C8) = 1;
            } else {
                H(actor, 0x3C8) = 0;
            }

            if (abs_angle_diff(B(actor, 0x3AB), B(actor, 0x3AA)) < 0x11U &&
                W(actor, 0x3A0) >= 0x20U) {
                B(actor, 0x397) = 0x12;
                break;
            }

            row = (unsigned int)B(actor, 0x392) * 12U + (unsigned int)B(actor, 0x398) * 264U;
            B(actor, 0x3D0) = (unsigned char)(func_8001E6B8((9U - (D_8010C32B[row] & 0xFU)) << 1) + 3U);
            B(actor, 0x397) = (unsigned char)(B(actor, 0x397) + 1);
            break;

        case 17:
            B(actor, 0x3D1) = 5;
            H(actor, 0x3AE) = (unsigned short)(H(actor, 0x3AE) - 2);
            func_8005BD1C(actor);

            angle_target = func_8005B36C(S(actor, 0x2), S(actor, 0x6), S(Scratchpad, 0xFE), S(Scratchpad, 0x100));
            func_8003172C(actor, angle_target & 0xFFU);

            dx = S(actor, 0x2) - S(Scratchpad, 0xFE);
            dz = S(actor, 0x6) - S(Scratchpad, 0x100);
            dist = func_800B1128((unsigned int)(dx * dx + dz * dz));

            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            angle_misc = func_8005B36C(S(actor, 0x2), S(actor, 0x6), S(ball, 0x2), S(ball, 0x6));
            func_80048D94(actor, angle_target & 0xFFU, angle_misc & 0xFFU, dist & 0xFFFFU);

            if (dist < 0x40U) {
                func_8005BA30(actor, 1, 0);
                B(actor, 0x397) = 0x14;
            } else if (B(actor, 0x3D0) == 0 && W(actor, 0x3A0) == 0x30U) {
                B(actor, 0x3AB) = (unsigned char)angle_target;
                B(actor, 0x397) = 0x12;
            }
            break;

        case 18:
            B(actor, 0x3D1) = 5;
            H(actor, 0x3AE) = (unsigned short)(H(actor, 0x3AE) - 2);
            func_8005BD1C(actor);

            angle_target = func_8005B36C(S(actor, 0x2), S(actor, 0x6), S(Scratchpad, 0xFE), S(Scratchpad, 0x100));
            dx = S(actor, 0x2) - S(Scratchpad, 0xFE);
            dz = S(actor, 0x6) - S(Scratchpad, 0x100);
            dist = func_800B1128((unsigned int)(dx * dx + dz * dz));
            desired_heading = angle_target & 0xFFU;
            func_8003172C(actor, desired_heading);

            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            calc_speed = dist / (unsigned int)(S(ball, 0x3CC) + 1);
            if (H(actor, 0x38E) == 4) {
                calc_speed += 2;
            }

            if ((int)calc_speed >= 0x31) {
                unsigned int tmp_h;
                unsigned int new_h;
                unsigned int max_spd;

                func_80054A1C(actor);
                func_8005BA78(actor, 4, B(actor, 0x390));
                tmp_h = func_8005B7CC(desired_heading, B(actor, 0x3AA), 4);
                new_h = func_8005B7CC(desired_heading, tmp_h & 0xFFU, 4);
                B(actor, 0x3AA) = (unsigned char)new_h;
                B(actor, 0x3A4) = (unsigned char)new_h;

                max_spd = B(actor, 0x3B4);
                /* UNCLEAR: both branches of (calc_speed < max_spd) execute calc_speed = max_spd in delay slot */
                calc_speed = max_spd;
                W(actor, 0x3A0) = (unsigned int)((int)W(actor, 0x3A0) + ((int)calc_speed / (int)B(actor, 0x3B5)));
                if (calc_speed < W(actor, 0x3A0)) {
                    W(actor, 0x3A0) = calc_speed;
                }
            } else {
                if ((int)calc_speed < 0x29) {
                    calc_speed = 0x28;
                }
                angle_misc = func_8005B36C(S(actor, 0x2), S(actor, 0x6), S(Scratchpad, 0x38), S(Scratchpad, 0x68));
                func_80048D94(actor, desired_heading, angle_misc & 0xFFU, calc_speed & 0xFFFFU);
            }

            func_800484B8(S(actor, 0x2), S(actor, 0x6), B(actor, 0x3B4));

            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            ball_dx_fe = S(ball, 0x2) - S(Scratchpad, 0xFE);
            ball_dz_fe = S(ball, 0x6) - S(Scratchpad, 0x100);
            ball_dist_fe_sq = (unsigned int)(ball_dx_fe * ball_dx_fe + ball_dz_fe * ball_dz_fe);
            ball_dx_f8 = S(ball, 0x2) - S(Scratchpad, 0xF8);
            ball_dz_f8 = S(ball, 0x6) - S(Scratchpad, 0xFC);
            ball_dist_f8_sq = (unsigned int)(ball_dx_f8 * ball_dx_f8 + ball_dz_f8 * ball_dz_f8);

            if ((int)ball_dist_f8_sq < (int)ball_dist_fe_sq) {
                ball = (unsigned char *)W(Scratchpad, 0x2F4);
                angle_misc = func_8005B36C(S(actor, 0x2), S(actor, 0x6), S(ball, 0x2), S(ball, 0x6));
                if (abs_angle_diff(angle_misc, B(actor, 0x3AB)) < 0x10U) {
                    B(actor, 0x397) = 0x13;
                } else {
                    ball = (unsigned char *)W(Scratchpad, 0x2F4);
                    angle_misc = func_8005B36C(S(ball, 0x2), S(ball, 0x6), S(Scratchpad, 0xFE), S(Scratchpad, 0x100));
                    ball = (unsigned char *)W(Scratchpad, 0x2F4);
                    if (abs_angle_diff(angle_misc, B(ball, 0x3A4)) >= 0x41U ||
                        W(ball, 0x3A0) < 0x2000U) {
                        B(actor, 0x397) = 0x13;
                    } else if (dist < 0x40U) {
                        func_8005BA30(actor, 1, 0);
                        B(actor, 0x397) = 0x14;
                    }
                }
            } else {
                B(actor, 0x397) = 0x13;
            }
            func_80045504(actor, 0xC);
            break;

        case 19:
            B(actor, 0x3D1) = 5;
            H(actor, 0x3AE) = (unsigned short)(H(actor, 0x3AE) - 2);
            func_8005BD1C(actor);
            func_8004840C(actor);
            func_800479B8(actor, 1);
            func_80045504(actor, 0xC);
            break;

        case 20:
            func_8005BD1C(actor);
            H(actor, 0x3AE) = (unsigned short)(H(actor, 0x3AE) - 2);
            B(actor, 0x3D1) = 1;

            /* Goal-line X: +/- 13088 raw (+/- 52.352 m) */
            angle_target = func_8005B36C(S(actor, 0x2), S(actor, 0x6),
                                         (B(actor, 0x399) != 0) ? -0x3320 : 0x3320, 0);
            desired_heading = func_8005B36C(S(actor, 0x2), S(actor, 0x6),
                                            S(Scratchpad, 0x36), S(Scratchpad, 0x66));
            angle_target = func_8005B7CC(angle_target & 0xFFU, desired_heading & 0xFFU, 0x40);
            angle_misc = func_8005B7CC(angle_target & 0xFFU, B(actor, 0x3AA), 0x10);
            B(actor, 0x3AA) = (unsigned char)angle_misc;
            B(actor, 0x3A4) = (unsigned char)angle_misc;

            S(actor, 0x2) = (signed short)((S(actor, 0x2) + S(Scratchpad, 0xFE)) >> 1);
            S(actor, 0x6) = (signed short)((S(actor, 0x6) + S(Scratchpad, 0x100)) >> 1);

            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            dx = S(actor, 0x2) - S(ball, 0x2);
            dz = S(actor, 0x6) - S(ball, 0x6);
            dist_sq = (unsigned int)(dx * dx + dz * dz);

            if (abs_angle_diff(desired_heading, angle_target) < 9U ||
                abs_angle_diff(angle_target, B(ball, 0x3A4)) < 9U ||
                (S(ball, 0x3CC) < 2 && dist_sq > 0x90000U)) {
                B(actor, 0x397) = 0x13;
            }

            if (S(actor, 0x3CC) == 4 && dist_sq > 0x90000U) {
                B(actor, 0x396) = 4;
                B(actor, 0x3B1) = 0;
                H(actor, 0x3C2) = 0;
                B(actor, 0x3B0) = 0;
                B(actor, 0x397) = 1;
                break;
            }
            func_80045504(actor, 0xC);
            break;

        case 21:
            func_8005BD1C(actor);
            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            B(actor, 0x3AA) = (unsigned char)func_8005B7CC(B(ball, 0x3A4), B(actor, 0x3AA), 8);

            speed = W(actor, 0x3A0);
            if (speed < 0x13U) {
                W(actor, 0x3A0) = 0;
            } else {
                W(actor, 0x3A0) = speed - 0x12U;
            }

            if (B(actor, 0x3D0) == 0) {
                func_8005BA30(actor, 0x26, 0xF);
                next_state = S(actor, 0x3CC);
                if (next_state == 4) {
                    B(actor, 0x396) = 4;
                    B(actor, 0x3B1) = 0;
                    B(actor, 0x3B0) = 0;
                    H(actor, 0x3C2) = 0;
                    B(actor, 0x397) = 1;
                } else if (next_state == 0xFF) {
                    B(actor, 0x396) = 3;
                    B(actor, 0x397) = 2;
                } else {
                    B(actor, 0x397) = 0;
                    B(actor, 0x396) = (unsigned char)H(actor, 0x3CC);
                }
            }
            func_80045504(actor, 0xC);
            break;

        case 22:
            func_8005BD1C(actor);
            if (B(actor, 0x390) == 0) {
                func_8005BA30(actor, 4, 4);
                next_state = S(actor, 0x3CC);
                if (next_state == 4) {
                    B(actor, 0x396) = 4;
                    B(actor, 0x3B1) = 0;
                    B(actor, 0x3B0) = 0;
                    H(actor, 0x3C2) = 0;
                    B(actor, 0x397) = 1;
                } else if (next_state == 0xFF) {
                    B(actor, 0x396) = 3;
                    B(actor, 0x397) = 2;
                } else {
                    B(actor, 0x397) = 0;
                    B(actor, 0x396) = (unsigned char)H(actor, 0x3CC);
                }
            }
            W(actor, 0x3A0) += (((unsigned int)B(actor, 0x3B3) / 6U) & 0xFFU);
            if (W(actor, 0x3A0) > (unsigned int)B(actor, 0x3B3)) {
                W(actor, 0x3A0) = B(actor, 0x3B3);
            }
            func_80045504(actor, 0xC);
            break;

        default:
            break;
        }
    }

    if (B(Scratchpad, 0x2A9) == 0) {
        if (S(actor, 0x3C8) != 0) {
            func_800512B8(actor, 1);
        }
        if (B(actor, 0x39A) != 0) {
            side_off = (unsigned int)B(actor, 0x399) * 2U;
            prev_id = B(Scratchpad, 0x31E + side_off);
            B(Scratchpad, 0x31F + side_off) = (unsigned char)prev_id;
            if (prev_id != B(actor, 0x38D)) {
                B(Scratchpad, 0x31E + side_off) = B(actor, 0x38D);
            }
        }
    }

    if (S(actor, 0x3CC) == 4) {
        if (H(Scratchpad, 0x12 + (unsigned int)B(actor, 0x398) * 2U) & H(actor, 0x3D4)) {
            H(actor, 0x3C6) = (unsigned short)(H(actor, 0x3C6) + 2);
        }
        func_8002A83C(actor, B(actor, 0x3C6));
    }

    if (S(actor, 0x3CC) != 0xFF || S(actor, 0x3CA) != 1) {
        if (B(actor, 0x3D1) == 1) {
            B(actor, 0x3D1) = 2;
        }
    }

    func_8005B908(actor);
}
