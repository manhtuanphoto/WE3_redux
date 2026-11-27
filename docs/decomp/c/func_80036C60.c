/*
 * func_80036C60
 *
 * Purpose:
 *   Aerial ball intercept / jumping header and goalkeeper dive/jump handler.
 *   Evaluates the predicted ball trajectory in scratchpad, decides whether the
 *   actor (player or GK) can reach the ball in time and within reach height,
 *   initiates a jump/dive toward the intercept point, updates 3D velocity and
 *   facing during flight, checks ball contact, and returns to idle on landing.
 *
 * Arguments:
 *   actor (u8 *): pointer to 1000-byte actor struct (player or GK).
 *
 * Return value:
 *   void
 *
 * Scratchpad / Actor fields read & written:
 *   Scratchpad:
 *     0x46/0x5E/0x76 (s16): predicted ball x/y/z at frame 8 (0x4E+8*2 = 0x5E)
 *     0x4E.. (s16[]): predicted ball y table indexed by intercept frame
 *     0x60 (s16): predicted ball y at frame 9 (0x4E + 9*2)
 *     0xF0/0xF2/0xF4 (s16): actor predicted/target xyz (0xF2 = max reach y, negative = up)
 *     0xF8/0xFA/0xFC (s16): predicted ball intercept xyz (written by func_800489A8)
 *     0x290 (u32): frame counter
 *     0x2B0 (u32): ball event code
 *     0x2F4 (u8 *): pointer to ball actor
 *     0x31C (s16): frames since last ball touch
 *     0x31E + dir*2 (u8): goalkeeper actor id per attack direction
 *     0x322 + dir*2 (u8): nearest outfield/marker actor id per attack direction
 *   Actor:
 *     +0x02/+0x04/+0x06 (s16): x, y (negative = up), z
 *     +0x26 (s16): rotation
 *     +0x38D (u8): actor id
 *     +0x390 (u8): anim frame
 *     +0x392 (u8): team slot (0..10)
 *     +0x396 (u8): state (writes 0, 1, 0x1B, 0x25, 0x26, 0x28)
 *     +0x397 (u8): substate (0..5)
 *     +0x398 (u8): side (0/1)
 *     +0x399 (u8): attack direction flag
 *     +0x39B (u8): flag checked on landing
 *     +0x3A4 (u8): move heading
 *     +0x3A7 (u8): active flag (written 1)
 *     +0x3AA (u8): body heading
 *     +0x3AB (u8): target heading
 *     +0x3AC (u8): mirror / foot flag
 *     +0x3B4 (u8): max speed (read on opponent actor)
 *     +0x3C2 (s16/u16): target actor id (0 = goal/ball intercept)
 *     +0x3C4 (s16): timer/random value (set when going to state 0x1B)
 *     +0x3CA/+0x3CC/+0x3CE (s16): jump velocity x/y/z
 *     +0x3D1 (u8): aerial intercept phase flag (1 -> 3)
 *
 * Units & magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x3330 (13104) = ~52.42 m (pitch half-length clamp)
 *   0x3320 (13088) = ~52.35 m (goal line)
 *   0x331F (13087) = ~52.35 m
 *   0x2E07 (11783) = ~47.13 m (near goal area threshold)
 *   0x23E2 (9186)  = ~36.74 m (penalty box x threshold, ~15.6 m from goal line)
 *   0x198F (6543)  = ~26.17 m (midfield/halfway threshold)
 *   0x12F4/0x12F5 (4852/4853) = ~19.41 m (penalty box half-width)
 *   0x8B7 (2231)   = ~8.92 m (goal area half-width)
 *   0x3B2 (946)    = ~3.78 m (goal mouth half-width)
 *   -0x27F (-639)  = ~2.56 m height (negative = up; goal crossbar height)
 *   -0x60 (-96)    = ~0.384 m extra GK reach height
 *   0x40 (64)      = ~0.256 m vertical reach tolerance
 *   0x240000       = 1536^2 raw (~6.14 m radius squared)
 *   0x100000       = 1024^2 raw (~4.10 m radius squared)
 *   0x40000        = 512^2 raw (~2.05 m radius squared)
 *   0x24000        = 384^2 raw (~1.54 m radius squared)
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned char D_8010C32F[];
extern unsigned char D_8010C330[];

extern unsigned int func_8001E5A4(unsigned int angle, int scale);
extern unsigned int func_8001E694(unsigned int angle, int scale);
extern int func_8001E6B8(int n);
extern void func_800363B8(unsigned char *actor);
extern unsigned int func_800381F4(unsigned char *actor);
extern void func_80038F18(unsigned char *actor, int a1, int a2, int a3);
extern void func_800391B8(unsigned char *actor);
extern unsigned int func_8003CEE4(unsigned char *actor, unsigned int frame, int a2, int a3, unsigned int a4);
extern unsigned int func_8003D274(unsigned char *actor, int a1, int a2);
extern unsigned int func_800489A8(void);
extern unsigned int func_800494C0(unsigned char *actor);
extern void func_800498AC(unsigned char *actor);
extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);
extern unsigned int func_8005B7CC(unsigned int target, unsigned int cur, unsigned int step);
extern void func_8005B908(unsigned char *actor);
extern void func_8005BA30(unsigned char *actor, int anim, int frame);
extern void func_8005BD1C(unsigned char *actor);
extern void func_8005C2AC(unsigned char *actor, int anim, int a2, unsigned int heading, unsigned int mirror);
extern void func_8005C454(unsigned char *ptr, int a1, int a2);
extern unsigned int func_8005CE14(unsigned int actor_id);
extern void func_800676C4(unsigned int dir, int x, int z, int a3);
extern unsigned int func_800B1128(int val);

void func_80036C60(unsigned char *actor)
{
    unsigned char *scratch = Scratchpad;
    unsigned char substate = B(actor, 0x397);
    unsigned char frame;
    signed short adj_x;
    signed short pred_x;
    signed short pred_z;
    unsigned int dive_dir;
    int anim;
    int reach_y;
    unsigned int stat_word;
    int stat_val;
    int a3_arg;
    unsigned int sp10_arg;

    B(actor, 0x3A7) = 1;

    if (substate >= 6) {
        func_800498AC(actor);
        return;
    }

    switch (substate) {
    case 0: {
        signed short fc;
        signed short abs_fc;
        signed short abs_z;
        unsigned char opp_idx;
        unsigned char *opp_actor;
        int dx, dz;
        unsigned int dist, time;
        int dist_sq;

        B(actor, 0x3D1) = 1;
        stat_word = W(D_8010C330, B(actor, 0x392) * 12 + B(actor, 0x398) * 264);
        stat_val = (int)(((stat_word >> 11) & 0x7F) + 100) * 592;
        reach_y = (-stat_val) / 180;
        if (B(actor, 0x38D) == B(scratch, 0x31E + B(actor, 0x399) * 2)) {
            reach_y -= 0x60;
        }
        S(scratch, 0xF2) = (signed short)reach_y;

        frame = (unsigned char)func_800489A8();
        pred_x = S(scratch, 0xF8);
        adj_x = B(actor, 0x399) ? (signed short)(-pred_x) : pred_x;

        if (adj_x < -0x331F && S(scratch, 0xFA) < -0x27F) {
            func_8005BD1C(actor);
            S(actor, 0x3C4) = (signed short)(func_8001E6B8(0xC0) + 0xC0);
            B(actor, 0x396) = 0x1B;
            B(actor, 0x397) = 0x18;
            func_8005B908(actor);
            func_800498AC(actor);
            return;
        }

        if (adj_x < -0x23E2) {
            abs_z = S(actor, 0x6);
            if (abs_z < 0) {
                abs_z = -abs_z;
            }
            if (abs_z < 0x12F4) {
                B(actor, 0x396) = 0x25;
            }
        }

        fc = S(scratch, 0xFC);
        abs_fc = fc < 0 ? (signed short)(-fc) : fc;
        if (abs_fc < 0x12F5 && adj_x < -0x23E1) {
            B(actor, 0x397)++;
            goto case_1_entry;
        }

        func_800676C4(B(actor, 0x399) ^ 1, S(scratch, 0xF8), fc, 0);
        opp_idx = B(scratch, 0x322 + (B(actor, 0x399) ^ 1) * 2);
        opp_actor = D_800FF748 + (opp_idx % 24) * 1000;
        dx = S(opp_actor, 0x2) - S(scratch, 0xF8);
        dz = S(opp_actor, 0x6) - S(scratch, 0xFC);
        dist = func_800B1128(dx * dx + dz * dz);
        time = dist / B(opp_actor, 0x3B4);

        if ((unsigned int)(frame + 8) < time && frame >= 0x11) {
            dx = S(opp_actor, 0x2) - S(scratch, 0xF8);
            dz = S(opp_actor, 0x6) - S(scratch, 0xFC);
            dist_sq = dx * dx + dz * dz;
            if (dist_sq > 0x240000 || W(scratch, 0x2B0) == 0xB) {
                B(actor, 0x396) = (B(actor, 0x396) == 0x24) ? 0x26 : 0x28;
                B(actor, 0x397) = 0;
                func_8005BD1C(actor);
                func_80038F18(actor, 0, 0, 0);
                func_8005B908(actor);
                func_800498AC(actor);
                return;
            }
        }

        B(actor, 0x397)++;
    }
    /* fall through */

    case 1:
    case_1_entry: {
        signed short threshold;
        int dx, dz;
        signed short actor_x;
        signed short adj_actor_x;

        func_8005BD1C(actor);
        B(actor, 0x3D1) = 1;

        if (S(scratch, 0x31C) == 1) {
            B(actor, 0x396) = 1;
            B(actor, 0x397) = 0;
            func_8005B908(actor);
            func_800498AC(actor);
            return;
        }

        if (B(actor, 0x38D) != (func_8005CE14(B(actor, 0x38D)) & 0xFF)) {
            int in_own_half = B(actor, 0x399) ? (S(actor, 0x2) > 0) : (S(actor, 0x2) < 0);
            if (in_own_half && S(W(scratch, 0x2F4), 0x3DC) == 2) {
                B(actor, 0x396) = 1;
                B(actor, 0x397) = 0;
                func_8005B908(actor);
                func_800498AC(actor);
                return;
            }
        }

        stat_word = W(D_8010C330, B(actor, 0x392) * 12 + B(actor, 0x398) * 264);
        stat_val = (int)(((stat_word >> 11) & 0x7F) + 100) * 592;
        reach_y = (-stat_val) / 180;
        if (B(actor, 0x38D) == B(scratch, 0x31E + B(actor, 0x399) * 2)) {
            reach_y -= 0x60;
        }
        S(scratch, 0xF2) = (signed short)reach_y;

        stat_word = W(D_8010C330, B(actor, 0x392) * 12 + B(actor, 0x398) * 264);
        stat_val = (int)(((stat_word >> 11) & 0x7F) + 100) * 592;
        threshold = (signed short)(((-stat_val) / 180) - 0x60);

        if (S(scratch, 0x5E) < threshold) {
            actor_x = S(actor, 0x2);
            dx = actor_x - S(scratch, 0x46);
            dz = S(actor, 0x6) - S(scratch, 0x76);
            if ((dx * dx + dz * dz) > 0x24000 && S(scratch, 0x5E) < S(scratch, 0xF2)) {
                func_80038F18(actor, 0, 0, 0);
                func_8005B908(actor);
                func_800498AC(actor);
                return;
            }
            adj_actor_x = B(actor, 0x399) ? (signed short)(-actor_x) : actor_x;
            if (adj_actor_x >= -0x23E2) {
                if ((B(actor, 0x399) & 1) != (W(scratch, 0x290) & 1)) {
                    func_80038F18(actor, 0, 0, 0);
                    func_8005B908(actor);
                    func_800498AC(actor);
                    return;
                }
            }
        }
    }
    /* fall through */

    case 2: {
        unsigned char nearest_id;
        unsigned char *other_actor;
        unsigned char dir;
        unsigned char my_id;
        unsigned char gk_id;
        unsigned char opp_gk_id;
        unsigned char *opp_gk;
        unsigned char *my_gk;
        int dx, dz;
        signed short target_id;
        unsigned int ang_to_ball, ang_to_pred;
        unsigned int diff;
        unsigned char *ball;
        unsigned int speed, max_speed;
        unsigned char stat_nibble;
        unsigned int move_angle;
        int denom;
        signed short abs_z;

        nearest_id = func_8005CE14(B(actor, 0x38D)) & 0xFF;
        other_actor = D_800FF748 + (nearest_id % 24) * 1000;
        if (B(other_actor, 0x396) == 0x8C || B(other_actor, 0x396) == 0x90) {
            dx = S(actor, 0x2) - S(other_actor, 0x2);
            dz = S(actor, 0x6) - S(other_actor, 0x6);
            if (dx * dx + dz * dz <= 0x23FFF) {
                B(actor, 0x396) = 0x1B;
                B(actor, 0x397) = 0x18;
                func_8005B908(actor);
                func_800498AC(actor);
                return;
            }
        }

        frame = 8;
        pred_x = S(scratch, 0x46);
        pred_z = S(scratch, 0x76);

        if (SW(W(scratch, 0x2F4), 0xC) > 0) {
            dir = B(actor, 0x399);
            my_id = B(actor, 0x38D);
            gk_id = B(scratch, 0x31E + dir * 2);
            if (my_id != gk_id) {
                opp_gk_id = B(scratch, 0x31E + (dir ^ 1) * 2);
                opp_gk = D_800FF748 + (opp_gk_id % 24) * 1000;
                dx = S(opp_gk, 0x2) - pred_x;
                dz = S(opp_gk, 0x6) - pred_z;
                if (dx * dx + dz * dz > 0x100000) {
                    my_gk = D_800FF748 + (gk_id % 24) * 1000;
                    dx = S(actor, 0x2) - S(my_gk, 0x2);
                    dz = S(actor, 0x6) - S(my_gk, 0x6);
                    if (dx * dx + dz * dz <= 0x3FFFF) {
                        B(actor, 0x396) = 0x1B;
                        B(actor, 0x397) = 0x18;
                        func_8005B908(actor);
                        func_800498AC(actor);
                        return;
                    }
                }
            }
        }

        if (B(actor, 0x396) == 0x25) {
            S(actor, 0x3C2) = 0;
        } else {
            S(actor, 0x3C2) = (signed short)(func_800381F4(actor) & 0xFF);
        }

        target_id = S(actor, 0x3C2);
        if (target_id == 0) {
            dive_dir = (unsigned int)B(actor, 0x399) << 7;
        } else {
            unsigned char *target_actor = D_800FF748 + (target_id % 24) * 1000;
            dive_dir = func_8005B36C(S(actor, 0x2), S(actor, 0x6), S(target_actor, 0x2), S(target_actor, 0x6));
        }

        if (S(actor, 0x3C2) == 0) {
            dx = S(actor, 0x2) - pred_x;
            dz = S(actor, 0x6) - pred_z;
            if ((dx * dx + dz * dz) / (int)(frame + 1) >= 0x1C) {
                ball = (unsigned char *)W(scratch, 0x2F4);
                ang_to_ball = func_8005B36C(S(actor, 0x2), S(actor, 0x6), S(ball, 0x2), S(ball, 0x6));
                ang_to_pred = func_8005B36C(S(actor, 0x2), S(actor, 0x6), pred_x, pred_z);
                if (((ang_to_ball - ang_to_pred) & 0xFF) < 0x81U) {
                    ball = (unsigned char *)W(scratch, 0x2F4);
                    ang_to_ball = func_8005B36C(S(actor, 0x2), S(actor, 0x6), S(ball, 0x2), S(ball, 0x6));
                    ang_to_pred = func_8005B36C(S(actor, 0x2), S(actor, 0x6), pred_x, pred_z);
                    if ((int)((ang_to_ball - ang_to_pred) & 0xFF) < 0x21) {
                        B(actor, 0x3AB) = (unsigned char)func_8005B36C(S(actor, 0x2), S(actor, 0x6), pred_x, pred_z);
                        goto after_heading_adjust;
                    }
                    B(actor, 0x3AB) = (unsigned char)dive_dir;
                } else {
                    ang_to_pred = func_8005B36C(S(actor, 0x2), S(actor, 0x6), pred_x, pred_z);
                    ball = (unsigned char *)W(scratch, 0x2F4);
                    ang_to_ball = func_8005B36C(S(actor, 0x2), S(actor, 0x6), S(ball, 0x2), S(ball, 0x6));
                    if ((int)((ang_to_pred - ang_to_ball) & 0xFF) < 0x21) {
                        B(actor, 0x3AB) = (unsigned char)func_8005B36C(S(actor, 0x2), S(actor, 0x6), pred_x, pred_z);
                        goto after_heading_adjust;
                    }
                    B(actor, 0x3AB) = (unsigned char)dive_dir;
                }
            } else {
                B(actor, 0x3AB) = (unsigned char)dive_dir;
            }
        } else {
            B(actor, 0x3AB) = (unsigned char)dive_dir;
        }

        {
            unsigned char ball_heading = B((unsigned char *)W(scratch, 0x2F4), 0x3A4);
            unsigned int opp_dive = dive_dir + 0x80;
            unsigned char step;
            diff = (ball_heading - opp_dive) & 0xFF;
            if (diff < 0x81U) {
                step = (unsigned char)(diff / 3U);
            } else {
                step = (unsigned char)(((opp_dive - ball_heading) & 0xFF) / 3U);
            }
            B(actor, 0x3AB) = (unsigned char)func_8005B7CC(
                B(actor, 0x3AB),
                (B((unsigned char *)W(scratch, 0x2F4), 0x3A4) + 0x80) & 0xFF,
                step);
        }

    after_heading_adjust:
        if (pred_x >= 0x3331) {
            pred_x = 0x3330;
        } else if (pred_x < -0x3330) {
            pred_x = -0x3330;
        }

        adj_x = B(actor, 0x399) ? (signed short)(-pred_x) : pred_x;
        if (adj_x < -0x23E2) {
            anim = (B(actor, 0x38D) == B(scratch, 0x31E + B(actor, 0x399) * 2)) ? 0x32 : 0x24;
        } else {
            anim = (B(actor, 0x399) != 0) ? 0x24 : 0x32;
        }

        func_8005C2AC(actor, anim, 0, B(actor, 0x3AB), B(actor, 0x3AC));

        move_angle = func_8005B36C(S(scratch, 0xF0), S(scratch, 0xF4), pred_x, pred_z);
        dx = S(scratch, 0xF0) - pred_x;
        dz = S(scratch, 0xF4) - pred_z;
        denom = (int)frame + 1;
        speed = func_800B1128(dx * dx + dz * dz) / (unsigned int)denom;

        stat_nibble = B(D_8010C32F, B(actor, 0x392) * 12 + B(actor, 0x398) * 264) & 0xF;
        diff = (B(actor, 0x3A4) - move_angle) & 0xFF;
        if (diff < 0x81U) {
            max_speed = (unsigned int)((stat_nibble + 0x16) - ((int)diff >> 5));
        } else {
            max_speed = (unsigned int)((stat_nibble + 0x16) - ((int)((move_angle - B(actor, 0x3A4)) & 0xFF) >> 5));
        }
        if (max_speed < speed) {
            speed = max_speed;
        }

        move_angle &= 0xFF;
        S(actor, 0x3CA) = (signed short)func_8001E694(move_angle, speed);
        S(actor, 0x3CC) = (signed short)((S(scratch, 0x4E + frame * 2) - S(scratch, 0xF2)) / denom);
        stat_nibble = B(D_8010C32F, B(actor, 0x392) * 12 + B(actor, 0x398) * 264) & 0xF;
        func_8005C454(actor + 0x3CC, 0xC, (signed short)((speed >> 8) - (stat_nibble - 5)));
        S(actor, 0x3CE) = (signed short)func_8001E5A4(move_angle, speed);

        S(scratch, 0xF0) += S(actor, 0x3CA) * denom;
        S(scratch, 0xF2) += S(actor, 0x3CC) * denom;
        S(scratch, 0xF4) += S(actor, 0x3CE) * denom;

        adj_x = B(actor, 0x399) ? (signed short)(-S(actor, 0x2)) : S(actor, 0x2);
        abs_z = S(actor, 0x6) < 0 ? (signed short)(-S(actor, 0x6)) : S(actor, 0x6);
        if ((adj_x < -0x2E07 && abs_z < 0x8B7) ||
            (adj_x < -0x23E2 && abs_z < 0x12F5 && func_800494C0(actor) <= 0x23FFFFU)) {
            unsigned char opp_gk_idx = B(scratch, 0x31E + (B(actor, 0x399) ^ 1) * 2);
            unsigned char *opp_gk_ptr = D_800FF748 + (opp_gk_idx % 24) * 1000;
            signed short adj_pred = B(actor, 0x399) ? (signed short)(-pred_x) : pred_x;
            signed short abs_pz = pred_z < 0 ? (signed short)(-pred_z) : pred_z;

            if (adj_pred < -0x3320 && abs_pz < 0x3B2) {
                a3_arg = 0xFF;
            } else {
                int d_actor = (S(actor, 0x2) - pred_x) * (S(actor, 0x2) - pred_x) +
                              (S(actor, 0x6) - pred_z) * (S(actor, 0x6) - pred_z);
                int d_opp = (S(opp_gk_ptr, 0x2) - pred_x) * (S(opp_gk_ptr, 0x2) - pred_x) +
                            (S(opp_gk_ptr, 0x6) - pred_z) * (S(opp_gk_ptr, 0x6) - pred_z);
                a3_arg = (d_opp < d_actor) ? 0xFF : 0;
            }

            stat_word = W(D_8010C330, B(actor, 0x392) * 12 + B(actor, 0x398) * 264);
            stat_val = (int)(((stat_word >> 11) & 0x7F) + 100) * 592;
            if (S(scratch, 0x60) < (-stat_val) / 180) {
                sp10_arg = 0x9000;
            } else if (B(actor, 0x38D) == B(scratch, 0x31E + B(actor, 0x399) * 2)) {
                sp10_arg = 0x40000;
            } else {
                sp10_arg = 0x9000;
            }
        } else {
            if (adj_x >= -0x198F) {
                sp10_arg = 0x14400;
                a3_arg = 0xFF;
            } else {
                sp10_arg = 0x9000;
                a3_arg = 0;
            }
        }

        if ((func_8003CEE4(actor, frame, 1, a3_arg, sp10_arg) & 0xFF) != 0 &&
            S(scratch, 0x5E) < S(scratch, 0xF2) + 0x40) {
            func_8005BA30(actor, anim, 0);
            B(actor, 0x3D1) = 3;
            B(actor, 0x3A4) = (unsigned char)move_angle;
            ball = (unsigned char *)W(scratch, 0x2F4);
            S(actor, 0x4) += S(actor, 0x3CC);
            S(actor, 0x2) += S(actor, 0x3CA);
            S(actor, 0x6) += S(actor, 0x3CE);
            B(actor, 0x397) = 3;
            B(actor, 0x3AC) = ((((B(actor, 0x3AB) - B(ball, 0x3A4)) - 0x80) & 0xFF) < 0x81U) ^ 1;
        } else {
            stat_word = W(D_8010C330, B(actor, 0x392) * 12 + B(actor, 0x398) * 264);
            stat_val = (int)(((stat_word >> 11) & 0x7F) + 100) * 592;
            if (S(scratch, 0x60) < (-stat_val) / 180) {
                func_80038F18(actor, 0, 0, 0);
            } else {
                B(actor, 0x396) = (B(actor, 0x396) == 0x24) ? 0x26 : 0x28;
                B(actor, 0x397) = 0;
            }
            func_8005B908(actor);
        }
        break;
    }

    case 3: {
        unsigned int heading;
        int rot;

        func_8005BD1C(actor);
        heading = func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 0xC);
        B(actor, 0x3AA) = (unsigned char)heading;
        rot = 0x1C0 - (int)(heading & 0xFF);
        S(actor, 0x26) = (signed short)((rot % 256) << 4);
        S(actor, 0x2) += S(actor, 0x3CA);
        S(actor, 0x6) += S(actor, 0x3CE);
        S(actor, 0x4) += S(actor, 0x3CC);
        if (B(actor, 0x390) == 8) {
            B(actor, 0x397)++;
            S(actor, 0x3CC) = (signed short)((S(actor, 0x3CC) * 3) / 2);
            if (func_8003D274(actor, 1, 1) & 0xFF) {
                func_800391B8(actor);
            }
        }
        break;
    }

    case 4: {
        unsigned int heading;
        int rot;
        signed short vx, vz;

        func_8005BD1C(actor);
        if (B(actor, 0x390) == 9) {
            if (func_8003D274(actor, 1, 1) & 0xFF) {
                func_800391B8(actor);
            }
        }
        heading = func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 8);
        B(actor, 0x3AA) = (unsigned char)heading;
        rot = 0x1C0 - (int)(heading & 0xFF);
        S(actor, 0x26) = (signed short)((rot % 256) << 4);

        vx = S(actor, 0x3CA);
        vz = S(actor, 0x3CE);
        S(actor, 0x2) += vx;
        S(actor, 0x6) += vz;
        S(actor, 0x4) = (signed short)((S(actor, 0x4) * 8) / 10);
        S(actor, 0x3CA) = (signed short)((vx * 9) / 10);
        S(actor, 0x3CE) = (signed short)((vz * 9) / 10);

        if (B(actor, 0x390) >= 0xF) {
            S(actor, 0x4) = 0;
            if (B(actor, 0x39B) != 0 && func_8001E6B8(3) == 0) {
                func_800363B8(actor);
            } else {
                B(actor, 0x397)++;
            }
        }
        break;
    }

    case 5:
        func_8005BD1C(actor);
        if (B(actor, 0x390) == 0) {
            func_8005BA30(actor, 1, 0);
            B(actor, 0x396) = 0;
            B(actor, 0x397) = 0;
        }
        break;
    }

    func_800498AC(actor);
}
