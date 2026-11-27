/*
 * func_8004C39C
 *
 * Purpose:
 *   Evaluates and resolves a player's ball contact / first-touch / trap / aerial
 *   header-or-volley attempt against the ball's height, distance, and owner
 *   state; delegates special touches (func_8004E2A0, func_8004DEFC,
 *   func_8004DA18, func_8004D464) or executes ground trap / deflection /
 *   aerial header or volley kick.
 *
 * Arguments:
 *   actor       - pointer to the player's 1000-byte actor struct.
 *   max_dist_sq - squared horizontal contact radius limit when the ball is
 *                 free or owned by this actor (and not a fresh teammate pass).
 *
 * Return value:
 *   1 if ball contact/action was handled, 0 if out of reach/range.
 *
 * Actor fields read/written:
 *   Reads:  +0x02 (x), +0x06 (z), +0x38D (actor id), +0x38E (anim id),
 *           +0x390 (anim frame), +0x392 (team slot), +0x396 (state),
 *           +0x398 (side), +0x399 (attack dir), +0x39A (CPU flag),
 *           +0x3A0 (speed), +0x3A4 (move heading), +0x3AA (body heading),
 *           +0x3D1 (action flag), +0x3D4 (controller input bits)
 *   Writes: +0x26 (rotation), +0x396 (state), +0x397 (substate),
 *           +0x3A0 (speed = 0), +0x3AA (body heading), +0x3AC (mirror/foot),
 *           +0x3B0 (8 or 11), +0x3B1 (0), +0x3B2 (0xFF), +0x3C2/+0x3C4 (0),
 *           +0x3C6/+0x3C8 (saved x/z), +0x3D0 (0 or 4), +0x3D1 (0)
 *   Ball actor (Scratchpad+0x2F4):
 *     Reads  +0x02 (x), +0x04 (y, negative=up), +0x06 (z), +0x0C (vy),
 *            +0x3A4 (move heading), +0x3C2 (target x), +0x3C6 (target z)
 *     Writes +0x02/+0x04/+0x06 (snapped position), +0x3A4 (move heading),
 *            +0x3B0 (4 or 6), +0x3B1 (0), +0x3B2 (6)
 *
 * Scratchpad fields read/written:
 *   Reads:  0x28C (input dir index), 0x2A9 (ball owner), 0x2AA (prev owner),
 *           0x2AB (prev-prev owner), 0x2B0 (ball event), 0x2CF[2] (possession),
 *           0x2E7 (pad mode), 0x2F4 (ball actor ptr), 0x31C (ball timer),
 *           0x328 (CPU flag)
 *   Writes: 0x28C (input dir index), 0x2A9 (ball owner), 0x2AA/0x2AB (owner history),
 *           0x2B0 (0), 0x2B4/0x2B8 (0x7FFF), 0x2CF[2] (possession), 0x2F2 (0),
 *           0x31C (0), 0x332 (3)
 *
 * Magic numbers / unit conversions (250 raw = 1 m, 30 frames/s):
 *   Reach height = -((stat + 100) * 460) / 180 (approx -1.02 m to -2.03 m up)
 *   0x2401 = 9217 (dist >= 96 raw = 0.384 m); 0x4001 = 16385 (dist >= 128 raw = 0.512 m)
 *   0x190000 = 1638400 (1280^2 raw, 5.12 m radius^2)
 *   0x68 = 104 raw (0.416 m ball offset); 0x300 = 768 raw (3.072 m lookahead)
 *   -0xA0 = -160 raw (0.64 m up); -0x74 = -116 raw (0.464 m up)
 *   -0xBF = -191 raw (0.764 m up); -0xE0 = -224 raw (0.896 m up)
 *   0x3120 = 12576 raw (50.304 m); 0x1F00 = 7936 raw (31.744 m)
 *   -0x28EF = -10479 raw (-41.916 m); 0x420 = 1056 raw (4.224 m)
 *   0x3981 = 14721 (vertical velocity threshold); 736 = gravity/lift divisor
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned char D_8010C32C[];
extern unsigned char D_8010C330[];
extern unsigned char D_800C6B74[];

extern void func_800512B8(unsigned char *actor, int flag);
extern int func_8005B36C(int x1, int z1, int x2, int z2);
extern int func_8001E694(unsigned int angle, int scale);
extern int func_8001E5A4(unsigned int angle, int scale);
extern void func_8004E2A0(unsigned char *actor, unsigned int angle);
extern unsigned int func_8004DEFC(unsigned char *actor, unsigned int angle);
extern int func_8004DA18(unsigned char *actor);
extern unsigned int func_8005E0E8(unsigned int dir);
extern void func_800676C4(unsigned int atk_dir, int x, int z, int flag);
extern unsigned int func_80049D94(unsigned char *actor);
extern void func_80054A1C(unsigned char *actor);
extern void func_8005BA30(unsigned char *actor, int anim, int frame);
extern int func_8001E6B8(int n);
extern int func_8005C7A0(int range);
extern void func_8004B0F0(unsigned char *actor, unsigned int heading, unsigned int power, int lift);
extern unsigned int func_80052C6C(unsigned char *actor);
extern int func_80055610(unsigned char *actor, unsigned char *prev_actor, int power);
extern int func_8005B7CC(unsigned int target, unsigned int cur, int step);
extern void func_8005BA78(unsigned char *actor, int anim, int frame);
extern void func_8004D464(unsigned char *actor, unsigned int ball_ang, unsigned int want_ang, int power);
extern unsigned int func_80049398(unsigned char *actor, unsigned int want_ang, int radius_sq);
extern void func_80045504(unsigned char *actor, int arg1);

int func_8004C39C(unsigned char *actor, unsigned int max_dist_sq)
{
    unsigned char *ball;
    unsigned char side;
    unsigned char slot;
    unsigned char owner;
    unsigned char prev_owner;
    int reach_y;
    int dx, dz;
    int dist_sq;
    int target_ang;
    int ball_ang = 0; /* UNCLEAR: $s4 is uninitialized on the aerial path if func_8004D464 is called */
    int want_ang;
    int power;
    int lift;
    int stat;
    unsigned int heading;
    int turn;

    slot = B(actor, 0x392);
    side = B(actor, 0x398);
    {
        unsigned int h_stat = (W(D_8010C330, side * 264 + slot * 12) >> 11) & 0x7F;
        reach_y = -((int)(h_stat + 100) * 460) / 180;
    }

    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    if (reach_y >= S(ball, 0x4)) {
        return 0;
    }

    owner = B(Scratchpad, 0x2A9);
    if (B(actor, 0x38D) == owner || owner == 0) {
        if (B(Scratchpad, 0x2CF + side) == 1 &&
            W(Scratchpad, 0x2B0) == 1 &&
            S(Scratchpad, 0x31C) < 16) {
            dx = S(actor, 0x2) - S(ball, 0x2);
            dz = S(actor, 0x6) - S(ball, 0x6);
            dist_sq = dx * dx + dz * dz;
            if (dist_sq >= 0x2401) { /* >= 0.384 m */
                return 0;
            }
        } else {
            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            dx = S(actor, 0x2) - S(ball, 0x2);
            dz = S(actor, 0x6) - S(ball, 0x6);
            if (max_dist_sq < (unsigned int)(dx * dx + dz * dz)) {
                return 0;
            }
        }
    } else {
        dx = S(actor, 0x2) - S(ball, 0x2);
        dz = S(actor, 0x6) - S(ball, 0x6);
        dist_sq = dx * dx + dz * dz;
        if (dist_sq >= 0x4001) { /* >= 0.512 m */
            return 0;
        }
    }

    func_800512B8(actor, 1);

    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    target_ang = func_8005B36C(S(actor, 0x2), S(actor, 0x6), S(ball, 0x3C2), S(ball, 0x3C6));
    {
        unsigned char body = B(actor, 0x3AA);
        unsigned int diff = (unsigned int)(target_ang - body) & 0xFF;
        int in_front;
        if (diff < 0x81) {
            in_front = ((int)diff < 0x49);
        } else {
            in_front = ((int)((body - target_ang) & 0xFF) < 0x49);
        }

        if (!in_front) {
            int cur_ang;
            int cur_ang2;
            int ball_in_front;
            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            cur_ang = func_8005B36C(S(actor, 0x2), S(actor, 0x6), S(ball, 0x2), S(ball, 0x6));
            diff = (unsigned int)(cur_ang - B(actor, 0x3AA)) & 0xFF;
            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            cur_ang2 = func_8005B36C(S(actor, 0x2), S(actor, 0x6), S(ball, 0x2), S(ball, 0x6));
            if (diff < 0x81) {
                ball_in_front = ((int)((cur_ang2 - B(actor, 0x3AA)) & 0xFF) < 0x49);
            } else {
                ball_in_front = ((int)((B(actor, 0x3AA) - cur_ang2) & 0xFF) < 0x49);
            }
            if (!ball_in_front) {
                ball = (unsigned char *)W(Scratchpad, 0x2F4);
                if (S(ball, 0x4) < -0xA0) { /* higher than 0.64 m */
                    unsigned int ang_b = (unsigned int)target_ang & 0xFF;
                    H(ball, 0x2) = H(actor, 0x2) + func_8001E694(ang_b, 0x68);
                    ball = (unsigned char *)W(Scratchpad, 0x2F4);
                    H(ball, 0x6) = H(actor, 0x6) + func_8001E5A4(ang_b, 0x68);
                    func_8004E2A0(actor, ang_b);
                    return 1;
                }
            }
        }
    }

    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    B(ball, 0x3B1) = 0;
    B(Scratchpad, 0x2F2) = 0;
    S(Scratchpad, 0x2B4) = 0x7FFF;
    S(Scratchpad, 0x2B8) = 0x7FFF;

    if ((func_8004DEFC(actor, (unsigned int)target_ang & 0xFF) & 0xFF) != 0) {
        return 1;
    }
    if (func_8004DA18(actor) != 0) {
        return 1;
    }

    B(Scratchpad, 0x2CF + B(actor, 0x398)) = 0;
    B(Scratchpad, 0x2CF + (B(actor, 0x398) ^ 1)) = 1;
    if (B(actor, 0x3D1) == 5) {
        return 1;
    }

    if (B(actor, 0x39A) == 0) {
        unsigned char pad_mode = B(Scratchpad, 0x2E7);
        unsigned short d = H(actor, 0x3D4);
        if (pad_mode == 0) {
            B(Scratchpad, 0x28C) = ((d & 0x7000) >> 11) | (d >> 15);
        } else if (pad_mode == 0x80) {
            B(Scratchpad, 0x28C) = (d >> 13) | ((d & 0x1000) >> 9);
        } else {
            B(Scratchpad, 0x28C) = d >> 12;
        }
    } else {
        int x_abs = S(actor, 0x2);
        int z_abs = S(actor, 0x6);
        if (x_abs < 0) {
            x_abs = -x_abs;
        }
        if (z_abs < 0) {
            z_abs = -z_abs;
        }
        if (B(Scratchpad, 0x328) != 0 && x_abs < 0x3120 && z_abs < 0x1F00) {
            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            if ((B(actor, 0x399) != 0 ? (S(ball, 0x2) < 0) : (S(ball, 0x2) > 0)) ||
                W(Scratchpad, 0x2B0) == 1) {
                B(Scratchpad, 0x28C) = (func_8005E0E8((B(actor, 0x399) << 7) & 0x80) & 0xFFFF) >> 12;
            } else {
                B(Scratchpad, 0x28C) = 0;
            }
        } else {
            B(Scratchpad, 0x28C) = 0;
        }
    }

    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    if (S(ball, 0x4) >= -0x74) {
        /* Ground / low ball */
        prev_owner = B(Scratchpad, 0x2AA);
        if (B(actor, 0x38D) == prev_owner) {
            return 1;
        }
        B(Scratchpad, 0x2AB) = prev_owner;
        B(Scratchpad, 0x2AA) = B(actor, 0x38D);

        if (B(actor, 0x39A) != 0 && W(Scratchpad, 0x2B0) == 0xC) {
            int clear_x = (B(actor, 0x399) != 0) ? 0x1000 : -0x1000;
            func_800676C4(B(actor, 0x399), clear_x, 0, 0);
            S(actor, 0x3C2) = func_80049D94(actor) & 0xFF;
            S(actor, 0x3C4) = 0;
            B(Scratchpad, 0x332) = 3;
            B(actor, 0x396) = 5;
            B(actor, 0x397) = 0;
            S(Scratchpad, 0x31C) = 0;
            return 1;
        }

        if (B(actor, 0x3D1) == 1) {
            unsigned char dir = B(Scratchpad, 0x28C);
            int can_trap = 1;
            if (dir != 0) {
                unsigned char want = B(D_800C6B74, dir);
                unsigned int diff = (unsigned int)(B(actor, 0x3AA) - want) & 0xFF;
                if (diff < 0x81) {
                    if ((int)diff < 0x41) {
                        can_trap = 0;
                    }
                } else {
                    if ((int)((want - B(actor, 0x3AA)) & 0xFF) < 0x41) {
                        can_trap = 0;
                    }
                }
            }
            if (can_trap) {
                unsigned int stat_w;
                W(Scratchpad, 0x2B0) = 0;
                func_80054A1C(actor);
                if (H(actor, 0x38E) != 0x16) {
                    func_8005BA30(actor, 0x16, 0);
                }
                power = 0;
                stat_w = W(D_8010C330, B(actor, 0x398) * 264 + B(actor, 0x392) * 12);
                B(actor, 0x3D0) = 4;
                B(actor, 0x396) = 0xE;
                B(actor, 0x3D1) = 0;
                B(actor, 0x397) = 0x10;
                B(actor, 0x3AC) = (stat_w >> 4) & 1;
                goto ground_bounce_check;
            }
        }

        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        ball_ang = func_8005B36C(S(actor, 0x2), S(actor, 0x6), S(ball, 0x2), S(ball, 0x6));
        {
            unsigned int diff = (unsigned int)(ball_ang - B(actor, 0x3AA)) & 0xFF;
            int close_ang;
            if (diff < 0x81) {
                close_ang = ((int)diff < 0x31);
            } else {
                close_ang = ((int)((B(actor, 0x3AA) - ball_ang) & 0xFF) < 0x31);
            }
            if (close_ang) {
                unsigned int ang_b = (unsigned int)ball_ang & 0xFF;
                H(ball, 0x2) = H(actor, 0x2) + func_8001E694(ang_b, 0x68);
                ball = (unsigned char *)W(Scratchpad, 0x2F4);
                H(ball, 0x6) = H(actor, 0x6) + func_8001E5A4(ang_b, 0x68);
            }
        }
        power = (int)W(actor, 0x3A0);

    ground_bounce_check:
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        {
            int vy_abs = SW(ball, 0xC);
            if (vy_abs < 0) {
                vy_abs = -vy_abs;
            }
            if (vy_abs >= 0x3981 && S(Scratchpad, 0x31C) < 0x21) {
                stat = (W(D_8010C32C, B(actor, 0x398) * 264 + B(actor, 0x392) * 12) >> 12) & 0xF;
                if (func_8001E6B8(9 - stat) != 0) {
                    int vy_half;
                    stat = (W(D_8010C32C, B(actor, 0x398) * 264 + B(actor, 0x392) * 12) >> 12) & 0xF;
                    power += 0xC + func_8001E6B8((9 - stat) << 2);
                    if (power < 0x38) {
                        power = 0x38;
                    } else if (power >= 0x53) {
                        power = 0x52;
                    }
                    ball = (unsigned char *)W(Scratchpad, 0x2F4);
                    vy_half = SW(ball, 0xC) >> 1;
                    if (vy_half < 0) {
                        vy_half = -vy_half;
                    }
                    lift = func_8001E6B8(vy_half / 736);
                    func_8004B0F0(actor, (unsigned int)(B(actor, 0x3AA) + func_8005C7A0(8)) & 0xFF,
                                  (unsigned int)power, lift);
                    B((unsigned char *)W(Scratchpad, 0x2F4), 0x3B0) = 4;
                    if (B(actor, 0x396) != 0xE && (func_80052C6C(actor) & 0xFF) == 0) {
                        B(actor, 0x396) = 0x1B;
                        B(actor, 0x397) = 1;
                    }
                    goto ground_finish;
                }
            }
        }

        {
            unsigned char *prev_actor = D_800FF748 + (B(Scratchpad, 0x2AB) % 24) * 1000;
            if (func_80055610(actor, prev_actor, power) != 0) {
                stat = (W(D_8010C32C, B(actor, 0x398) * 264 + B(actor, 0x392) * 12) >> 12) & 0xF;
                power += 0x10 + func_8001E6B8(9 - stat);
                if (power < 0x34) {
                    power = 0x34;
                } else if (power >= 0x53) {
                    power = 0x52;
                }
                lift = func_8001E6B8(4);
                func_8004B0F0(actor, B(actor, 0x3AA), (unsigned int)power, lift);
                S(Scratchpad, 0x31C) = 0;
                B(Scratchpad, 0x2A9) = B(actor, 0x38D);
                B((unsigned char *)W(Scratchpad, 0x2F4), 0x3B0) = 4;
                if ((func_80052C6C(actor) & 0xFF) == 0) {
                    B(actor, 0x396) = 0x1B;
                    B(actor, 0x397) = 1;
                }
                return 1;
            }
        }

    ground_finish:
        B((unsigned char *)W(Scratchpad, 0x2F4), 0x3A4) = B(actor, 0x3AA);
        W(Scratchpad, 0x2B0) = 0;
        S(Scratchpad, 0x31C) = 0;
        return 1;
    }

    /* Aerial ball (S(ball, 0x4) < -0x74, above 0.464 m) */
    S(Scratchpad, 0x31C) = 0;
    stat = (W(D_8010C32C, B(actor, 0x398) * 264 + B(actor, 0x392) * 12) >> 12) & 0xF;
    if (W(actor, 0x3A0) >= 0x31) {
        power = (int)W(actor, 0x3A0) + func_8001E6B8(9 - stat) + 6;
    } else {
        power = func_8001E6B8(9 - stat) + 0x38;
    }

    if (B(actor, 0x39A) == 0) {
        unsigned char dir = B(Scratchpad, 0x28C);
        want_ang = (dir != 0) ? B(D_800C6B74, dir) : B(actor, 0x3AA);
    } else {
        want_ang = B(actor, 0x399) << 7;
    }

    if (B(actor, 0x38D) == B(Scratchpad, 0x2AA)) {
        heading = (unsigned int)func_8005B7CC((unsigned int)target_ang & 0xFF, B(actor, 0x3AA), 0x10);
        B(actor, 0x3AA) = (unsigned char)heading;
        turn = 0x1C0 - (int)(heading & 0xFF);
        S(actor, 0x26) = (signed short)((turn % 256) << 4);

        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        lift = (S(ball, 0x4) < -0xBF) ? -7 : -5;
        func_8004B0F0(actor, B(actor, 0x3AA), (unsigned int)(power - 6), lift);
        B(Scratchpad, 0x2A9) = B(actor, 0x38D);
        B((unsigned char *)W(Scratchpad, 0x2F4), 0x3B0) = 6;
        B((unsigned char *)W(Scratchpad, 0x2F4), 0x3B2) = 6;
        if (B(actor, 0x396) == 1) {
            B(actor, 0x3D0) = 0;
            B(actor, 0x397) = 2;
        } else if (B(actor, 0x396) == 3) {
            func_8005BA78(actor, 4, B(actor, 0x390));
            B(actor, 0x397) = 2;
        }
        return 1;
    }

    {
        int x_check = (B(actor, 0x399) != 0) ? -S(actor, 0x2) : S(actor, 0x2);
        int z_abs = S(actor, 0x6);
        if (z_abs < 0) {
            z_abs = -z_abs;
        }
        if (x_check < -0x28EF && z_abs < 0x420) {
            ball_ang = B(actor, 0x399) << 7;
            goto aerial_header_or_volley;
        }
    }

    if (B(actor, 0x3D1) != 1) {
        func_8004D464(actor, (unsigned int)ball_ang & 0xFF, (unsigned int)want_ang & 0xFF, power);
        return 1;
    }

    if (B(Scratchpad, 0x28C) != 0) {
        unsigned char move_h = B(actor, 0x3A4);
        unsigned char body_h = B(actor, 0x3AA);
        unsigned int d1 = (unsigned int)(move_h - body_h) & 0xFF;
        int move_ok = (d1 < 0x81) ? ((int)d1 < 0x21) : ((int)((body_h - move_h) & 0xFF) < 0x21);
        if (move_ok) {
            unsigned int d2 = (unsigned int)(body_h - want_ang) & 0xFF;
            int want_ok = (d2 < 0x81) ? ((int)d2 < 0x21) : ((int)((want_ang - body_h) & 0xFF) < 0x21);
            if (want_ok && W(actor, 0x3A0) >= 0x30) {
                func_8004D464(actor, (unsigned int)ball_ang & 0xFF, (unsigned int)want_ang & 0xFF, power);
                return 1;
            }
        }
    }

    ball_ang = (int)func_80049398(actor, (unsigned int)want_ang & 0xFF, 0x190000);

aerial_header_or_volley:
    {
        unsigned int aim = (unsigned int)ball_ang & 0xFF;
        unsigned int step_ang;
        int look_x, look_z;
        int ang_to_ball;

        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        ang_to_ball = func_8005B36C(S(actor, 0x2), S(actor, 0x6), S(ball, 0x3C2), S(ball, 0x3C6));
        step_ang = (unsigned int)func_8005B7CC(aim, (unsigned int)ang_to_ball & 0xFF, 0x28) & 0xFF;

        H(ball, 0x2) = H(actor, 0x2) + func_8001E694(step_ang, 0x68);
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        H(ball, 0x4) = H(ball, 0x4) + 0x20;
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        H(ball, 0x6) = H(actor, 0x6) + func_8001E5A4(step_ang, 0x68);

        heading = (unsigned int)func_8005B7CC(step_ang, B(actor, 0x3AA), 0x28);
        B(actor, 0x3AA) = (unsigned char)heading;
        turn = 0x1C0 - (int)(heading & 0xFF);
        S(actor, 0x26) = (signed short)((turn % 256) << 4);

        look_x = (signed short)(H(actor, 0x2) + func_8001E694(aim, 0x300));
        look_z = (signed short)(H(actor, 0x6) + func_8001E5A4(aim, 0x300));
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        ball_ang = func_8005B7CC((unsigned int)func_8005B36C(S(ball, 0x2), S(ball, 0x6), look_x, look_z) & 0xFF,
                                 B(actor, 0x3AA), 0x38);

        stat = (W(D_8010C32C, B(actor, 0x398) * 264 + B(actor, 0x392) * 12) >> 12) & 0xF;
        {
            int base_pwr = func_8001E6B8((9 - stat) << 1) + 2;
            int rel;
            unsigned int r;
            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            rel = (ball_ang - B(ball, 0x3A4)) - 0x80;
            r = (unsigned int)rel & 0xFF;
            if (r < 0x81) {
                power = base_pwr + ((int)r / 12);
            } else {
                power = base_pwr + ((int)((-rel) & 0xFF) / 12);
            }
        }
    }

    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    if (S(ball, 0x4) < -0xE0) { /* header (above 0.896 m) */
        func_8005BA30(actor, 0x15, 0);
        B(actor, 0x3B0) = 0xB;
        lift = func_8001E6B8(4) - 0xA;
    } else { /* volley */
        B(actor, 0x3AC) = ((unsigned int)(B(ball, 0x3A4) - B(actor, 0x3AA)) & 0xFF) < 0x81;
        func_8005BA30(actor, 0x17, 0);
        B(actor, 0x3B0) = 8;
        lift = func_8001E6B8(2) - 8;
        {
            int vy_q = SW(ball, 0xC) >> 2;
            if (vy_q < 0) {
                vy_q = -vy_q;
            }
            power += func_8001E6B8(vy_q / 736);
        }
    }

    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    {
        int vy_q = SW(ball, 0xC) >> 2;
        if (vy_q < 0) {
            vy_q = -vy_q;
        }
        func_8004B0F0(actor, (unsigned int)ball_ang & 0xFF, (unsigned int)power,
                      lift + func_8001E6B8(vy_q / 736));
    }

    B(Scratchpad, 0x2A9) = B(actor, 0x38D);
    B(actor, 0x3B2) = 0xFF;
    B(actor, 0x3D0) = 4;
    B(actor, 0x3D1) = 0;
    B(actor, 0x3B1) = 0;
    B(actor, 0x396) = 0xE;
    B(actor, 0x397) = 0;
    W(actor, 0x3A0) = 0;
    H(actor, 0x3C6) = H(actor, 0x2);
    H(actor, 0x3C8) = H(actor, 0x6);
    func_80045504(actor, 6);
    return 1;
}
