/*
 * func_8002D9F4
 *
 * Purpose:
 *   Executes a player's pass / clearance / long-ball kick depending on the pass
 *   mode in actor+0x3C4: computes target position and pass angle/power/lift
 *   (via func_8002F2B0 / func_8002EB3C / func_8002F6C8), kicks the ball with
 *   func_8004B0F0, snaps the ball to the scratchpad kick origin, notifies the
 *   receiving teammate (and optionally an opponent or offside-trap player),
 *   and sets ball event/ownership state.
 *
 * Arguments:
 *   actor - pointer to the kicking player's 1000-byte actor struct.
 *
 * Return value:
 *   None (void).
 *
 * Actor fields read/written:
 *   Reads:  +0x02 (x), +0x06 (z), +0x08 (vx), +0x10 (vz), +0x38D (actor id),
 *           +0x38E (anim id), +0x392 (team slot), +0x394 (stamina/fatigue),
 *           +0x398 (side), +0x399 (attack dir), +0x39B (flag), +0x3AA (body heading),
 *           +0x3B0 (flag), +0x3C2 (target id), +0x3C4 (pass mode),
 *           +0x3C6 (x lead offset), +0x3C8 (z lead offset), +0x3CA (pass dist),
 *           +0x3CC (pass angle), +0x3CE (pressure/charge?)
 *   Writes: +0x3C2 (target id), +0x3C4 (pass mode reset to 0 or 6),
 *           +0x3C6 (random lead jitter in modes 1/4), +0x3D1 (3)
 *   Target/other actors (D_800FF748 + (id % 24) * 1000):
 *     Writes +0x396 (state: 0x1B or 0x1D), +0x397 (substate: 0 or 0x40), +0x3C2
 *   Ball actor (Scratchpad+0x2F4):
 *     Reads  +0x02 (x), +0x06 (z), +0x3A4 (move heading)
 *     Writes +0x02 (x), +0x04 (y), +0x06 (z), +0x396 (4), +0x3B0 (4), +0x3B2 (2)
 *
 * Scratchpad fields read/written:
 *   Reads:  0x36 (kick x), 0x4E (kick y), 0x66 (kick z), 0xFE/0x100 (target xz),
 *           0x29A (match phase), 0x2F4 (ball actor ptr), 0x307[side^1], 0x322[4]
 *   Writes: 0xFE/0x100 (updated target xz), 0x2B0 (ball event = 1),
 *           0x31C (ball timer / wait), 0x332 (0)
 *
 * Magic numbers / unit conversions (250 raw = 1 m, 30 frames/s):
 *   0x1201 = 4609 raw (18.436 m); 0x1401 = 5121 raw (20.484 m)
 *   0x28F0 = 10480 raw (41.92 m); 0x12F4 = 4852 raw (19.408 m)
 *   0x3320 = 13088 raw (52.352 m, goal line); 0x800 = 2048 raw (8.192 m)
 *   0x1000 = 4096 raw (16.384 m); 0x800 = 2048 raw (8.192 m)
 *   0x508 = sound id; 0x13 = anim id (19)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned char D_8010C32E[];
extern unsigned char D_800C6C6C[];

extern unsigned int func_800B1128(int val);
extern int func_8005B61C(int x1, int z1, int x2, int z2);
extern int func_8005C7A0(int range);
extern int func_8002ECB0(unsigned char *actor, unsigned int heading, unsigned int dist);
extern int func_8001E6B8(int n);
extern int func_8002F6C8(int dist);
extern int func_8002F2B0(unsigned char *target, unsigned int heading, unsigned int min_pwr,
                         unsigned int max_pwr, int max_spd, int cap_pwr, int dist,
                         int lead_steps, int lift);
extern void func_800676C4(unsigned int atk_dir, int x, int z, int flag);
extern unsigned int func_80049D94(unsigned char *actor);
extern unsigned int func_80052C6C(unsigned char *actor);
extern unsigned int func_8002EB3C(int dist, int power);
extern unsigned int func_80052D90(unsigned char *actor);
extern void func_800512B8(unsigned char *actor, int flag);
extern int func_8005B36C(int x1, int z1, int x2, int z2);
extern int func_8001E694(unsigned int angle, int scale);
extern int func_8001E5A4(unsigned int angle, int scale);
extern void func_8004B0F0(unsigned char *actor, unsigned int heading, unsigned int power, int lift);
extern void func_800AB620(int sfx_id);
extern void func_8002F554(unsigned char *actor);
extern void func_80067860(unsigned int atk_dir, int x, int z, unsigned int heading, int arg4, int dist_sq);
extern unsigned int func_8005CE30(unsigned int actor_id);

void func_8002D9F4(unsigned char *actor)
{
    unsigned char *target = 0;
    unsigned char *ball;
    unsigned char *other;
    signed short target_id;
    signed short pass_mode;
    int tx, tz;
    int dx, dz;
    unsigned int dist = 0;
    int heading = 0;
    unsigned char orig_heading = 0;
    unsigned int pass_heading;
    int power = 0;
    int min_pwr;
    int lead_steps;
    int lift = 0;
    int stat;
    int i;

    target_id = S(actor, 0x3C2);
    if (target_id != 0) {
        target = D_800FF748 + (signed short)(target_id % 24) * 1000;
    } else {
        if (S(actor, 0x3C4) != 5) {
            S(actor, 0x3C4) = 6;
        }
    }

    pass_mode = S(actor, 0x3C4);
    switch (pass_mode) {
    case 0:
    case 2:
    case 3:
    default:
        tx = S(target, 0x2) + (SW(target, 0x8) << 2);
        if (B(actor, 0x399) != 0) {
            tx -= S(actor, 0x3C6);
        } else {
            tx += S(actor, 0x3C6);
        }
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        dx = S(ball, 0x2) - (signed short)tx;
        tz = H(target, 0x6) + (SW(target, 0x10) << 2);
        dz = S(ball, 0x6) - (signed short)tz;
        dist = func_800B1128(dx * dx + dz * dz);

        if ((unsigned short)(H(actor, 0x3C4) - 2) < 2 && dist >= 0x1201) { /* >= 18.436 m */
            S(actor, 0x3C4) = 0;
        }

        heading = func_8005B61C(S(Scratchpad, 0x36), S(Scratchpad, 0x66),
                                (signed short)tx, (signed short)tz);
        if (B(actor, 0x39B) != 0 || B(actor, 0x3B0) == 0) {
            stat = H(D_8010C32E, B(actor, 0x398) * 264 + B(actor, 0x392) * 12) & 0xF;
            heading += func_8005C7A0((signed short)(((9 - stat) << 1) + 8));
        }

        orig_heading = (unsigned char)heading;
        if (B(actor, 0x39B) == 0 && (B(actor, 0x3B0) != 0 || S(actor, 0x3CE) < 0x38)) {
            heading = func_8002ECB0(actor, (unsigned int)heading & 0xFF, dist);
        }

        power = (int)((dist >> 5) + 0x20);
        {
            unsigned int cap = B(D_800C6C6C, (unsigned short)(H(actor, 0x394) / 22)) + 0x58;
            if (cap < (unsigned int)power) {
                power = (int)cap;
            }
        }

        min_pwr = 0x98;
        if (B(actor, 0x39B) == 0 && B(actor, 0x3B0) != 0) {
            unsigned int atk_ang = B(actor, 0x399) << 7;
            unsigned int target_ang = B(target, 0x3A4);
            unsigned int diff = (target_ang - atk_ang) & 0xFF;
            min_pwr = 0xC8;
            if (diff < 0x81) {
                lead_steps = ((int)diff >> 4) + 2;
            } else {
                lead_steps = ((int)((atk_ang - target_ang) & 0xFF) >> 4) + 2;
            }
        } else {
            stat = H(D_8010C32E, B(actor, 0x398) * 264 + B(actor, 0x392) * 12) & 0xF;
            lead_steps = func_8001E6B8((9 - stat) << 2) + 2;
        }

        if (S(actor, 0x3CE) >= 0x20) {
            min_pwr -= (S(actor, 0x3CE) >> 2);
        }

        lift = func_8002F6C8((int)dist);
        if (B(actor, 0x3B0) == 0) {
            stat = H(D_8010C32E, B(actor, 0x398) * 264 + B(actor, 0x392) * 12) & 0xF;
            lift += func_8001E6B8(12 - stat);
        }

        pass_heading = (unsigned int)heading & 0xFF;
        power = func_8002F2B0(target, pass_heading, (unsigned int)min_pwr << 8,
                              (unsigned int)power << 8, 0x30, power + 8,
                              (int)dist, (signed short)lead_steps, lift);

        func_800676C4(B(actor, 0x399), S(Scratchpad, 0xFE), S(Scratchpad, 0x100), 1);
        other = D_800FF748 + ((func_80049D94(actor) & 0xFF) % 24) * 1000;
        if (target != other && (func_80052C6C(other) & 0xFF) == 0) {
            if (pass_heading == orig_heading && S(actor, 0x3C6) == 0) {
                B(other, 0x396) = 0x1D;
                B(other, 0x397) = 0;
            } else {
                target = other;
                S(actor, 0x3C2) = B(other, 0x38D);
            }
        }
        break;

    case 1:
    case 4:
        stat = H(D_8010C32E, B(actor, 0x398) * 264 + B(actor, 0x392) * 12) & 0xF;
        {
            int inv_stat = 9 - stat;
            if (inv_stat == 1) {
                H(actor, 0x3C6) = H(actor, 0x3C6) + func_8001E6B8(0x40);
            } else if (inv_stat == 2) {
                H(actor, 0x3C6) = H(actor, 0x3C6) + func_8001E6B8(0x80);
            } else if (inv_stat == 3) {
                H(actor, 0x3C6) = H(actor, 0x3C6) + 0x40 + func_8001E6B8(0x80);
            } else if (inv_stat != 0) {
                H(actor, 0x3C6) = H(actor, 0x3C6) + 0x40 + func_8001E6B8(inv_stat << 6);
            }
        }

        if (B(actor, 0x399) != 0) {
            tx = S(target, 0x2) - S(actor, 0x3C6);
        } else {
            tx = S(target, 0x2) + S(actor, 0x3C6);
        }
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        tz = (signed short)(H(target, 0x6) + H(actor, 0x3C8));
        dx = S(ball, 0x2) - (signed short)tx;
        dz = S(ball, 0x6) - (signed short)tz;
        dist = func_800B1128(dx * dx + dz * dz);

        heading = func_8005B61C(S(Scratchpad, 0x36), S(Scratchpad, 0x66),
                                (signed short)tx, (signed short)tz);
        if (B(actor, 0x39B) != 0 || B(actor, 0x3B0) == 0 || S(actor, 0x3CE) >= 0x40) {
            stat = H(D_8010C32E, B(actor, 0x398) * 264 + B(actor, 0x392) * 12) & 0xF;
            heading += func_8005C7A0((signed short)(((9 - stat) << 1) + 8));
            if (S(actor, 0x3C4) == 1) {
                stat = H(D_8010C32E, B(actor, 0x398) * 264 + B(actor, 0x392) * 12) & 0xF;
                dist += (unsigned int)func_8001E6B8((9 - stat) << 6);
            }
        }
        if (dist >= 0x1401) { /* >= 20.484 m */
            dist += (unsigned int)func_8001E6B8((dist - 0x400) >> 3);
        }

        heading = func_8002ECB0(actor, (unsigned int)heading & 0xFF, dist + 0x600);
        min_pwr = B(D_800C6C6C, (unsigned short)(H(actor, 0x394) / 22)) + 0xC8;
        if (S(actor, 0x3CE) >= 0x20) {
            min_pwr -= (S(actor, 0x3CE) >> 2);
        }

        lead_steps = func_8002F6C8((int)dist);
        stat = H(D_8010C32E, B(actor, 0x398) * 264 + B(actor, 0x392) * 12) & 0xF;
        lift = lead_steps + func_8001E6B8(9 - stat);

        if (B(actor, 0x39B) == 0 && B(actor, 0x3B0) != 0 && S(actor, 0x3CE) < 0x38) {
            stat = H(D_8010C32E, B(actor, 0x398) * 264 + B(actor, 0x392) * 12) & 0xF;
            lead_steps = 4 - func_8001E6B8((9 - stat) * 3 + 2);
        } else {
            stat = H(D_8010C32E, B(actor, 0x398) * 264 + B(actor, 0x392) * 12) & 0xF;
            lead_steps = 4 - func_8001E6B8((9 - stat) * 6 + 4);
            lift += func_8001E6B8(3);
        }

        power = func_8002F2B0(target, (unsigned int)heading & 0xFF,
                              (unsigned int)min_pwr << 8, 0x4800,
                              B(target, 0x3B4), 0x40, (int)dist,
                              (signed short)lead_steps, lift);

        {
            int new_fe = S(Scratchpad, 0xFE);
            if (B(actor, 0x399) != 0) {
                new_fe += 0x40;
            } else {
                new_fe -= 0x40;
            }
            S(Scratchpad, 0xFE) = (signed short)new_fe;
            func_800676C4(B(actor, 0x399), (signed short)new_fe, S(Scratchpad, 0x100), 0);
        }
        other = D_800FF748 + ((func_80049D94(actor) & 0xFF) % 24) * 1000;
        if (target != other && (func_80052C6C(other) & 0xFF) == 0) {
            B(other, 0x396) = 0x1D;
            B(other, 0x397) = 0;
        }
        break;

    case 5:
        dist = (unsigned int)S(actor, 0x3CA);
        heading = B(actor, 0x3CC);
        power = func_8002EB3C((int)dist, func_8001E6B8(0x1800) + 0x4800) & 0xFF;
        lift = func_8002F6C8((int)dist);

        func_800676C4(B(actor, 0x399), S(Scratchpad, 0xFE), S(Scratchpad, 0x100), 0);
        target = D_800FF748 + ((func_80049D94(actor) & 0xFF) % 24) * 1000;
        if ((func_80052D90(target) & 0xFF) == 0) {
            func_800512B8(target, 1);
            B(target, 0x396) = 0x1B;
            B(target, 0x397) = 0;
            S(target, 0x3C2) = 0x10;
        }

        func_800676C4(B(actor, 0x399) ^ 1, S(Scratchpad, 0xFE), 0, 0);
        {
            unsigned char opp_id = B(Scratchpad, 0x322 + ((B(actor, 0x399) ^ 1) << 1));
            other = D_800FF748 + (opp_id % 24) * 1000;
            if ((func_80052D90(other) & 0xFF) == 0) {
                B(other, 0x396) = 0x1B;
                B(other, 0x397) = 0;
                S(other, 0x3C2) = 1;
            }
        }
        break;

    case 6:
        if (H(actor, 0x38E) == 0x13) {
            dist = 0x800; /* 8.192 m */
            power = 0x40;
        } else {
            dist = 0x1000; /* 16.384 m */
            power = 0x78;
        }

        {
            int x_check = (B(actor, 0x399) != 0) ? -S(actor, 0x2) : S(actor, 0x2);
            int z_abs = S(actor, 0x6);
            int in_clear_zone = 0;
            if (z_abs < 0) {
                z_abs = -z_abs;
            }
            if (x_check < -0x28F0 && z_abs < 0x12F4) { /* x beyond +-41.92 m, |z| < 19.408 m */
                int goal_x = (B(actor, 0x399) != 0) ? 0x3320 : -0x3320;
                int goal_ang = func_8005B36C(S(actor, 0x2), S(actor, 0x6), goal_x, 0);
                unsigned int diff = (unsigned int)(goal_ang - B(actor, 0x3CC)) & 0xFF;
                int goal_ang2 = func_8005B36C(S(actor, 0x2), S(actor, 0x6), goal_x, 0);
                if (diff < 0x81) {
                    in_clear_zone = ((int)((goal_ang2 - B(actor, 0x3CC)) & 0xFF) < 0x21);
                } else {
                    in_clear_zone = ((int)((B(actor, 0x3CC) - goal_ang2) & 0xFF) < 0x21);
                }
            }

            if (in_clear_zone) {
                signed short target_z = (S(actor, 0x6) >= 0) ? 0x800 : -0x800;
                int goal_x = (B(actor, 0x399) != 0) ? 0x3320 : -0x3320;
                heading = func_8005B36C(S(actor, 0x2), S(actor, 0x6), goal_x, target_z);
            } else if (H(actor, 0x38E) == 0x13) {
                heading = B(actor, 0x3AA) - 0x80;
            } else if ((B(Scratchpad, 0x29A) & 0xFF) == 1) {
                heading = func_8002ECB0(actor, B(actor, 0x3CC), dist);
            } else {
                ball = (unsigned char *)W(Scratchpad, 0x2F4);
                heading = B(ball, 0x3A4);
            }
        }

        lift = func_8002F6C8((int)dist);
        tx = H(actor, 0x2) + func_8001E694((unsigned int)heading & 0xFF, (int)dist);
        tz = H(actor, 0x6) + func_8001E5A4((unsigned int)heading & 0xFF, (int)dist);
        S(Scratchpad, 0xFE) = (signed short)tx;
        S(Scratchpad, 0x100) = (signed short)tz;
        func_800676C4(B(actor, 0x399), (signed short)tx, (signed short)tz, 0);
        target = D_800FF748 + ((func_80049D94(actor) & 0xFF) % 24) * 1000;
        B(target, 0x396) = 0x1B;
        B(target, 0x397) = 0;
        S(target, 0x3C2) = 1;
        break;
    }

    /* Common kick execution & teammate/offside notification */
    H(Scratchpad, 0x31C) = B(actor, 0x399) ^ 1;
    if (B(actor, 0x3B0) == 0) {
        stat = H(D_8010C32E, B(actor, 0x398) * 264 + B(actor, 0x392) * 12) & 0xF;
        H(Scratchpad, 0x31C) = H(Scratchpad, 0x31C) - 8 - func_8001E6B8(stat >> 1);
    }
    {
        unsigned int diff = (unsigned int)(heading - B(actor, 0x3AA)) & 0xFF;
        int turn_pen;
        if (diff < 0x81) {
            turn_pen = (int)diff >> 4;
        } else {
            turn_pen = (int)((B(actor, 0x3AA) - heading) & 0xFF) >> 4;
        }
        S(Scratchpad, 0x31C) = S(Scratchpad, 0x31C) - turn_pen;
    }

    if (H(actor, 0x38E) == 0x13 && (unsigned int)power >= 0x78) {
        power = 0x78;
    }
    pass_heading = (unsigned int)heading & 0xFF;
    func_8004B0F0(actor, pass_heading, (unsigned int)power, lift);

    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    H(ball, 0x2) = H(Scratchpad, 0x36);
    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    H(ball, 0x4) = H(Scratchpad, 0x4E);
    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    H(ball, 0x6) = H(Scratchpad, 0x66);
    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    B(ball, 0x396) = 4;
    W(Scratchpad, 0x2B0) = 1;
    B(Scratchpad, 0x332) = 0;
    func_800AB620(0x508);

    B(actor, 0x3D1) = 3;
    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    B(ball, 0x3B0) = 4;
    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    B(ball, 0x3B2) = 2;
    func_8002F554(actor);

    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    func_80067860(B(actor, 0x399), S(ball, 0x2), S(ball, 0x6), pass_heading, 0x10, (int)(dist * dist));

    for (i = 0; i < 2; i++) {
        unsigned char id = B(Scratchpad, 0x322 + (B(actor, 0x399) << 1) + i);
        if (id != 0xFF) {
            other = D_800FF748 + (id % 24) * 1000;
            if (B(other, 0x38D) != B(actor, 0x38D) &&
                B(other, 0x38D) != B(target, 0x38D) &&
                (func_80052C6C(other) & 0xFF) == 0 &&
                B(other, 0x38D) != 1 &&
                B(other, 0x38D) != 12) {
                B(other, 0x396) = 0x1B;
                B(other, 0x397) = 0x40;
            }
        }
    }

    if (B(Scratchpad, 0x307 + (B(actor, 0x398) ^ 1)) == 0) {
        unsigned int atk_ang = B(actor, 0x399) << 7;
        unsigned int diff = (atk_ang - (unsigned int)heading) & 0xFF;
        int is_forward;
        if (diff < 0x81) {
            is_forward = ((int)diff < 0x41);
        } else {
            is_forward = ((int)(((unsigned int)heading - atk_ang) & 0xFF) < 0x41);
        }
        if (is_forward) {
            unsigned char offside_id = (unsigned char)func_8005CE30(B(actor, 0x38D));
            func_800512B8(D_800FF748 + (offside_id % 24) * 1000, 0);
        }
    }
}
