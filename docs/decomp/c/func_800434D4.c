/*
 * func_800434D4
 *
 * Purpose:
 *   Evaluates and triggers defensive tackle/intercept/goal-line-clearance
 *   actions for a pressing defender (actor). Checks ball proximity, goal-mouth
 *   clearance conditions, opponent ball-carrier state/distance, and predicted
 *   tackle landing point (via func_8005C2AC), transitioning the actor into
 *   state 0x0C (intercept/block) or 0x0D (sliding/lunge tackle).
 *
 * Arguments:
 *   actor - pointer to 1000-byte actor struct
 *   angle - initial desired heading byte (0..255)
 *
 * Return value:
 *   int - 1 if a defensive action (state 0x0C or 0x0D) was initiated, 0 otherwise
 *
 * Actor fields read:
 *   - +0x02/+0x06 (s16): actor x/z
 *   - +0x38D (u8): actor ID
 *   - +0x38E (u16): anim ID (checked on teammate for 0x1E = sliding tackle)
 *   - +0x392 (u8): team slot (0..10, used to index D_8010C330)
 *   - +0x396 (u8): state
 *   - +0x398 (u8): side (0/1)
 *   - +0x399 (u8): attack direction flag (0/1)
 *   - +0x39A (u8): AI/CPU control flag
 *   - +0x3A4 (u8): move heading
 *   - +0x3AA (u8): body heading
 *   - +0x3B4 (u8): max speed cap
 *   - +0x3BC (s16): teammate action/availability check
 *
 * Actor fields written:
 *   - +0x08/+0x10 (s32): velocity x/z (when initiating sliding/lunge tackle)
 *   - +0x396 (u8): state (0x0C or 0x0D)
 *   - +0x397 (u8): substate (0, 1, or 0x20)
 *   - +0x3A4 (u8): move heading
 *   - +0x3AA (u8): body heading
 *   - +0x3AB (u8): target heading
 *   - +0x3AC (u8): mirror/foot flag
 *   - +0x3C2 (u16): target actor ID
 *   - +0x3C4 (u16): set to 1
 *   - +0x3C6 (s16): speed parameter
 *   - +0x3C8 (u16): set to 0 or 1
 *   - +0x3CA (s16): speed / parameter (8 or clamped d/7)
 *   - +0x3CC (u16): cleared to 0 (for state 0x0C)
 *   - +0x3D1 (u8): cleared to 0
 *
 * Scratchpad (0x1F800000) fields read:
 *   - +0x36/+0x66 (s16): target x/z
 *   - +0x3E/+0x6E (s16): ball predicted x/z
 *   - +0x42/+0x72 (s16): ball predicted x/z
 *   - +0x44/+0x74 (s16): ball predicted x/z
 *   - +0x5C (s16): predicted ball height
 *   - +0xF0/+0xF4 (s16): predicted actor landing x/z from func_8005C2AC
 *   - +0x2A9 (u8): ball owner ID (0 = free)
 *   - +0x2B0 (u32): ball event code
 *   - +0x2F4 (ptr): ball actor pointer
 *   - +0x31C (s16): ball timer (frames since last touch)
 *   - +0x31E/+0x31F[atk_dir*2] (u8): primary/secondary pressing defender IDs
 *   - +0x328 (u8): CPU pressing enable flag
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   - -0x32DF (-13023) -> -52.092 m x-coord (goal-line clearance threshold)
 *   - 0x3320 (13088) -> 52.352 m opponent goal x-coordinate
 *   - -0x1DE2 (-7650) -> -30.60 m x-coord (defending zone threshold)
 *   - 0x400 (1024) -> 4.096 m goal-mouth half-width
 *   - -0x100 (-256) -> -1.024 m ball height threshold (upward)
 *   - 0x2A3FFF (2768895, ~1664^2) -> 6.656 m radius squared
 *   - 0x190000 (1638400, 1280^2) -> 5.120 m radius squared
 *   - 0x40000 (262144, 512^2) -> 2.048 m radius squared
 *   - 0x23FFF (147455, ~384^2) -> 1.536 m radius squared
 *   - 0x381 (897) -> 3.588 m tackle landing distance threshold
 *   - 0x180 (384) -> 1.536 m lead offset ahead of ball
 *   - 0xF0 (240) -> 0.960 m lead offset ahead of ball
 *   - 0x100 (256) -> 1.024 m lateral offset
 *   - 0x31C < 9 -> < 0.30 s since last touch
 *   - Angles (0..255 = 360 deg): 0x18 (33.75 deg), 0x11 (23.9 deg),
 *     0x31/0x32 (68.9/70.3 deg), 0x40 (90 deg), 0x50/0x51 (112.5/113.9 deg),
 *     0x60 (135 deg)
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned char D_8010C330[];

extern int func_8001E5A4(unsigned int angle, int scale);
extern int func_8001E694(unsigned int angle, int scale);
extern int func_8001E6B8(int n);
extern unsigned int func_80049D94(unsigned char *actor);
extern int func_800550C8(unsigned char *actor);
extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);
extern unsigned int func_8005B7CC(unsigned int target, unsigned int cur, unsigned int step);
extern void func_8005BA30(unsigned char *actor, unsigned int anim, unsigned int frame);
extern void func_8005C2AC(unsigned char *actor, unsigned int anim, unsigned int frames,
                          unsigned int heading, unsigned int mirror);
extern unsigned int func_8005CE14(unsigned int actor_id);
extern void func_800676C4(unsigned int atk_dir, int x, int z, int arg3);
extern int func_800B1128(int val);

int func_800434D4(unsigned char *actor, unsigned int angle_arg)
{
    unsigned char angle = (unsigned char)angle_arg;
    unsigned char *pair;
    unsigned char actor_id;
    unsigned char *other;
    unsigned char *ball;
    int tx;
    int tz;
    int dx;
    int dz;
    unsigned int dist_sq;
    int ball_x;
    unsigned char opp_id;
    int opp_goal_x;
    unsigned int opp_goal_ang;
    unsigned char ang_diff_to_goal;
    int lead_x;
    int lead_z;
    unsigned int d_land;

    if (B(actor, 0x39A) != 0 && B(Scratchpad, 0x328) == 0) {
        return 0;
    }

    pair = Scratchpad + (B(actor, 0x399) << 1);
    actor_id = B(actor, 0x38D);
    if (actor_id != B(pair, 0x31E) && actor_id != B(pair, 0x31F)) {
        return 0;
    }

    func_800676C4(B(actor, 0x399), S(actor, 2), S(actor, 6), 0);

    other = D_800FF748 + ((func_80049D94(actor) & 0xFF) % 24) * 1000;
    if (B(other, 0x396) == 0xD) {
        return 0;
    }

    tx = S(Scratchpad, 0x44);
    tz = S(Scratchpad, 0x74);
    dx = S(actor, 2) - tx;
    dz = S(actor, 6) - tz;
    dist_sq = (unsigned int)(dx * dx + dz * dz);

    /* Goal-line clearance check when ball is free and just kicked */
    if (((B(actor, 0x399) != 0) ? (-tx < -0x32DF) : (tx < -0x32DF)) &&
        B(Scratchpad, 0x2A9) == 0 &&
        S(Scratchpad, 0x31C) < 9) {
        int abs_tz = (tz < 0) ? -tz : tz;
        if (abs_tz < 0x400 && S(Scratchpad, 0x5C) >= -0x100) {
            unsigned int ang1;
            unsigned int ang2;
            unsigned int diff;
            int ang_ok;

            other = D_800FF748 + ((func_8005CE14(B(actor, 0x38D)) & 0xFF) % 24) * 1000;
            ang1 = func_8005B36C(S(actor, 2), S(actor, 6), S(Scratchpad, 0x42), S(Scratchpad, 0x72));
            ang2 = func_8005B36C(S(actor, 2), S(actor, 6), S(other, 2), S(other, 6));

            if (((ang1 - ang2) & 0xFF) < 0x81) {
                ang_ok = (((ang1 - ang2) & 0xFF) >= 0x18);
            } else {
                ang_ok = (((ang2 - ang1) & 0xFF) >= 0x18);
            }

            if (ang_ok && dist_sq <= 0x2A3FFFU && S(other, 0x3BC) != 0) {
                unsigned char body = B(actor, 0x3AA);
                diff = (ang1 - body) & 0xFF;
                if ((diff < 0x81) ? (diff < 0x51) : (((body - ang1) & 0xFF) < 0x51)) {
                    B(actor, 0x396) = 0xD;
                    B(actor, 0x397) = 0;
                    return 1;
                }
            }
        }
    }

    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    ball_x = S(ball, 2);
    if (B(actor, 0x399) != 0) {
        ball_x = -ball_x;
    }
    if (ball_x >= -0x1DE2) {
        return 0;
    }

    opp_id = B(Scratchpad + ((B(actor, 0x399) ^ 1) << 1), 0x31E);
    other = D_800FF748 + (opp_id % 24) * 1000;
    if (B(other, 0x38D) != B(Scratchpad, 0x2A9)) {
        return 0;
    }

    {
        int bx = S(ball, 2);
        int bz = S(ball, 6);
        int ax = S(actor, 2) - bx;
        int az = S(actor, 6) - bz;
        int ox = S(other, 2) - bx;
        int oz = S(other, 6) - bz;
        if ((ox * ox + oz * oz) >= (ax * ax + az * az)) {
            return 0;
        }
    }

    opp_goal_x = (B(other, 0x399) != 0) ? -0x3320 : 0x3320;
    opp_goal_ang = func_8005B36C(S(other, 2), S(other, 6), opp_goal_x, 0);

    {
        unsigned char opp_state = B(other, 0x396);
        if (opp_state != 4 && opp_state != 9 && opp_state != 0x1E) {
            return 0;
        }
    }

    {
        unsigned int diff = (opp_goal_ang - angle) & 0xFF;
        ang_diff_to_goal = (diff < 0x81) ? (unsigned char)diff : (unsigned char)((angle - opp_goal_ang) & 0xFF);
    }

    if (func_800550C8(other) == 0) {
        return 0;
    }

    dx = S(actor, 2) - S(Scratchpad, 0x36);
    dz = S(actor, 6) - S(Scratchpad, 0x66);
    dist_sq = (unsigned int)(dx * dx + dz * dz);
    if (dist_sq > 0x190000U) {
        return 0;
    }

    lead_x = S(Scratchpad, 0x3E) + func_8001E694(opp_goal_ang & 0xFF, 0x180);
    lead_z = S(Scratchpad, 0x6E) + func_8001E5A4(opp_goal_ang & 0xFF, 0x180);
    angle = (unsigned char)func_8005B36C(S(actor, 2), S(actor, 6), (signed short)lead_x, (signed short)lead_z);

    if ((dist_sq <= 0x23FFFU && ang_diff_to_goal < 0x32) ||
        ang_diff_to_goal >= 0x60 ||
        B(other, 0x396) == 9 ||
        W(Scratchpad, 0x2B0) == 4) {
        /* Intercept / block (state 0x0C, substate 0x20) */
        int spd;
        unsigned int mirror;

        if (dist_sq > 0x40000U) {
            return 0;
        }

        lead_x = S(Scratchpad, 0x3E) + func_8001E694(opp_goal_ang & 0xFF, 0xF0);
        lead_z = S(Scratchpad, 0x6E) + func_8001E5A4(opp_goal_ang & 0xFF, 0xF0);
        angle = (unsigned char)func_8005B36C(S(actor, 2), S(actor, 6), (signed short)lead_x, (signed short)lead_z);

        if (func_8001E6B8(3) == 0 && B(other, 0x396) == 9) {
            angle = (unsigned char)func_8005B36C(S(actor, 2), S(actor, 6), S(Scratchpad, 0x3E), S(Scratchpad, 0x6E));
            lead_x = S(Scratchpad, 0x3E);
            lead_z = S(Scratchpad, 0x6E);
        }

        B(actor, 0x3D1) = 0;
        H(actor, 0x3C4) = 1;
        H(actor, 0x3C8) = 0;
        H(actor, 0x3C2) = B(Scratchpad, 0x2A9);

        mirror = (W(D_8010C330, B(actor, 0x398) * 264 + B(actor, 0x392) * 12) >> 4) & 1U;
        B(actor, 0x3AC) = (unsigned char)mirror;
        func_8005BA30(actor, 0xB, 0);

        dx = S(actor, 2) - (signed short)lead_x;
        dz = S(actor, 6) - (signed short)lead_z;
        spd = (int)((unsigned int)func_800B1128(dx * dx + dz * dz) / 5U);
        S(actor, 0x3C6) = (signed short)spd;
        if (B(actor, 0x3B4) < (signed short)spd) {
            H(actor, 0x3C6) = B(actor, 0x3B4);
        }

        B(actor, 0x3AA) = (unsigned char)func_8005B7CC(angle, B(actor, 0x3AA), 0x28);
        H(actor, 0x3CA) = 8;
        B(actor, 0x3AB) = angle;
        B(actor, 0x396) = 0xC;
        H(actor, 0x3CC) = 0;
        B(actor, 0x397) = 0x20;
        return 1;
    } else {
        /* Sliding or lunge tackle (state 0x0D, substate 1) */
        unsigned char closest_id;
        unsigned int max_spd;
        unsigned int turn_penalty;
        unsigned int mirror;
        unsigned int ang_to_target;
        unsigned int body_diff;
        unsigned int turn_heading;
        unsigned int tackle_spd;

        pair = Scratchpad + (B(actor, 0x399) << 1);
        closest_id = B(pair, 0x31E);
        if (B(actor, 0x38D) != closest_id) {
            if (ang_diff_to_goal < 0x31) {
                return 0;
            }
            other = D_800FF748 + (closest_id % 24) * 1000;
            if (H(other, 0x38E) == 0x1E) {
                int bx = S(ball, 2);
                int bz = S(ball, 6);
                int ax = S(actor, 2) - bx;
                int az = S(actor, 6) - bz;
                int ox = S(other, 2) - bx;
                int oz = S(other, 6) - bz;
                if ((ox * ox + oz * oz) < (ax * ax + az * az)) {
                    return 0;
                }
            }
        }

        other = D_800FF748 + ((func_8005CE14(B(actor, 0x38D)) & 0xFF) % 24) * 1000;
        {
            int mx = S(Scratchpad, 0x42);
            int mz = S(Scratchpad, 0x72);
            int mdx = S(other, 2) - mx;
            int mdz = S(other, 6) - mz;
            if ((mdx * mdx + mdz * mdz) <= 0x23FFF) {
                int out_ok = (B(actor, 0x399) != 0) ? (-mx < -0x32DF) : (mx < -0x32DF);
                if (!out_ok) {
                    return 0;
                }
            }
        }

        {
            unsigned int move_hdg = B(actor, 0x3A4);
            unsigned int diff = (move_hdg - angle) & 0xFF;
            if (diff < 0x81) {
                turn_penalty = (unsigned int)((int)diff >> 2);
            } else {
                turn_penalty = (unsigned int)((int)((angle - move_hdg) & 0xFF) >> 2);
            }
        }
        max_spd = (unsigned int)B(actor, 0x3B4) - turn_penalty;

        mirror = (W(D_8010C330, B(actor, 0x398) * 264 + B(actor, 0x392) * 12) >> 4) & 1U;
        func_8005C2AC(actor, 0x1E, 6, angle, mirror);

        dx = S(Scratchpad, 0xF0) - (signed short)lead_x;
        dz = S(Scratchpad, 0xF4) - (signed short)lead_z;
        d_land = (unsigned int)func_800B1128(dx * dx + dz * dz);
        if (d_land >= 0x381U) {
            return 0;
        }

        ang_to_target = func_8005B36C(S(Scratchpad, 0xF0), S(Scratchpad, 0xF4),
                                      (signed short)lead_x, (signed short)lead_z);
        body_diff = (ang_to_target - B(actor, 0x3AA)) & 0xFF;
        if (body_diff < 0x81) {
            if (body_diff >= 0x50) {
                return 0;
            }
        } else {
            if (((B(actor, 0x3AA) - ang_to_target) & 0xFF) >= 0x50) {
                return 0;
            }
        }

        pair = Scratchpad + (B(actor, 0x399) << 1);
        if (B(actor, 0x38D) == B(pair, 0x31E)) {
            func_8005BA30(actor, 0x1E, 0);
            B(actor, 0x3AC) = (unsigned char)((W(D_8010C330, B(actor, 0x398) * 264 + B(actor, 0x392) * 12) >> 4) & 1U);
        } else {
            int own_goal_x = (B(actor, 0x399) != 0) ? -0x3320 : 0x3320;
            unsigned int own_goal_ang;
            func_8005BA30(actor, 0x4D, 3);
            own_goal_ang = func_8005B36C(S(actor, 2), S(actor, 6), own_goal_x, 0);
            B(actor, 0x3AC) = (((own_goal_ang - angle) & 0xFF) < 0x81);
        }

        B(actor, 0x3AA) = (unsigned char)func_8005B7CC(angle, B(actor, 0x3AA), 8);

        if (ang_diff_to_goal < 0x11) {
            unsigned int perp_ang;
            if (((angle - opp_goal_ang) & 0xFF) < 0x81) {
                perp_ang = opp_goal_ang - 0x40;
            } else {
                perp_ang = opp_goal_ang + 0x40;
            }
            lead_x += func_8001E694(perp_ang & 0xFF, 0x100);
            lead_z += func_8001E5A4(perp_ang & 0xFF, 0x100);
        }

        turn_heading = func_8005B7CC(
            func_8005B36C(S(Scratchpad, 0xF0), S(Scratchpad, 0xF4),
                          (signed short)lead_x, (signed short)lead_z) & 0xFF,
            B(actor, 0x3AA), 0x28);

        tackle_spd = d_land / 7U;
        if (max_spd < tackle_spd) {
            tackle_spd = max_spd;
        }

        SW(actor, 8) = func_8001E694(turn_heading & 0xFF, (int)tackle_spd);
        SW(actor, 0x10) = func_8001E5A4(turn_heading & 0xFF, (int)tackle_spd);
        S(actor, 0x3CA) = (signed short)tackle_spd;
        B(actor, 0x3AB) = angle;
        B(actor, 0x3A4) = (unsigned char)turn_heading;
        B(actor, 0x3D1) = 0;
        H(actor, 0x3C4) = 1;
        H(actor, 0x3C8) = 1;
        B(actor, 0x396) = 0xD;
        B(actor, 0x397) = 1;
        H(actor, 0x3C6) = 0;
        H(actor, 0x3C2) = B(Scratchpad + ((B(actor, 0x399) ^ 1) << 1), 0x31E);
        return 1;
    }
}
