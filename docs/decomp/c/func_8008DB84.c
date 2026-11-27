/*
 * func_8008DB84
 *
 * Purpose:
 *   Evaluates whether a goalkeeper (or penalty-box defender) should initiate a
 *   dive, jump, or catch/punch save toward the predicted ball position 6 frames
 *   ahead (+0.20 s) when the ball is inside the penalty area. Checks opponent
 *   proximity/state, approach distance vs. stat-scaled reach thresholds, ball
 *   height, and ball travel angle, then sets the dive velocity, state
 *   (0x87, 0x88, 0x8A, 0x99, 0x9F, or 0xA1), and animation.
 *
 * Arguments:
 *   actor (a0) - pointer to 1000-byte player/goalkeeper actor struct
 *
 * Return value:
 *   int - 1 if a save/dive action was triggered, 0 otherwise
 *
 * Scratchpad (0x1F800000) fields read/written:
 *   0x036, 0x066 (s16, R): current ball x, z (frame +0)
 *   0x042, 0x05A, 0x072 (s16, R): predicted ball x, y, z (frame +6)
 *   0x044, 0x074 (s16, R): predicted ball x, z (frame +7)
 *   0x0F0, 0x0F4 (s16, R): scratch/reference x, z
 *   0x2A8 (u8, R): last-touch actor ID
 *   0x2A9 (u8, R): current ball owner (0 = free ball)
 *   0x2B0 (u32, R): ball event code (4 checked for high-speed shot/cross)
 *   0x2F4 (u8 *, R): pointer to ball actor
 *   0x31C (s16, R): ball timer (frames since last touch; 30 frames/s)
 *   0x31E (u8[], R): nearest/chasing actor ID indexed by (dir ^ 1) * 2
 *
 * Actor fields read/written:
 *   +0x002, +0x006 (s16, R/W): x, z position (remainder-adjusted on 0x99 dive)
 *   +0x004 (s16, W): height y (cleared to 0 on dive)
 *   +0x008, +0x010 (s32, W): x, z velocity (delta / 7)
 *   +0x38D (u8, R): actor ID
 *   +0x390 (u8, R): opponent animation frame (checked on opponent states)
 *   +0x392 (u8, R): team slot (0..10)
 *   +0x396 (u8, W): actor state (0x87, 0x88, 0x8A, 0x99, 0x9F, 0xA1)
 *   +0x397 (u8, W): actor substate (cleared to 0)
 *   +0x398 (u8, R): side (0/1)
 *   +0x399 (u8, R): attack direction flag (0/1)
 *   +0x3A0 (u32, R/W): speed (squared term used in reach check; cleared on dive)
 *   +0x3A4 (u8, R): ball move heading (0..255 = 360 deg)
 *   +0x3AA (u8, R): body heading
 *   +0x3AB (u8, R/W): target/save heading
 *   +0x3AC (u8, R/W): mirror/foot flag
 *   +0x3B0 (u8, W): ball/control timer (0 or 5)
 *   +0x3B9 (u8, R): GK response/stat value (divided by 10 for frame threshold)
 *   +0x3C2, +0x3C4, +0x3C6 (s16/u16, W): stored predicted ball x, y, z (frame +6)
 *   +0x3CC (s16, W): set to 8 on 0x88 dive
 *   +0x3D1 (u8, W): flag cleared to 0
 *
 * Globals read:
 *   D_800FF748 (u8[]): 1000-byte actor array
 *   D_8010C330 (u32[]): player stat table (stride 12 per slot, 264 per side; >> 11 & 0x7F)
 *
 * Magic numbers & unit conversions (250 raw = 1 m, 30 frames/s):
 *   0x2422 .. 0x3320 (9250 .. 13088 raw = 37.000 .. 52.352 m): penalty box X range
 *   0x12F5 (4853 raw = 19.412 m): penalty box half-width Z limit (|z| <= 4852 = 19.408 m)
 *   0x32A0 (12960 raw = 51.840 m): deep goal-line X check
 *   0x9000 (36864 sq dist -> sqrt ~192 raw = 0.768 m): minimum dist^2 to pred+6
 *   0x20 (32) = 45 deg, 0x24 (36) = 50.625 deg, 0x40 (64) = 90 deg,
 *   0x4C (76) = 106.875 deg, 0x80 (128) = 180 deg
 *   Height thresholds: (-176*stat)/180, (-432*stat)/180, (-480*stat)/180, (-608*stat)/180
 *   Reach thresholds: ((stat*240)/180)^2, ((stat*512)/180)^2
 *   Ball timers: 4 frames (0.133 s), 10 frames (0.333 s), 19/21 - stat/10 frames
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))

#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned char D_8010C330[];

extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);
extern void func_800956C0(unsigned char *actor, int param);
extern void func_8005C2AC(unsigned char *actor, int anim, int param2, int heading, int mirror);

int func_8008DB84(unsigned char *actor)
{
    signed short pred_x6;
    signed short pred_z6;
    signed short pred_y6;
    signed short pred_x7;
    signed short pred_z7;
    int abs_val;
    unsigned char opp_id;
    unsigned char *opp;
    signed short ball_x0;
    signed short ball_z0;
    int dx, dz;
    int dist_self;
    int dist_opp;
    unsigned int dist_pred6;
    unsigned int dist_pred7;
    unsigned int target_ang;
    unsigned char *ball;
    int diff;
    int aligned;
    int stat;
    int reach_num;
    int reach_div;
    unsigned int reach_sq;
    int height_thresh;
    int min_timer;
    unsigned int ang_to_ball0;
    unsigned char new_heading;
    int vx, vz;

    pred_x6 = S(Scratchpad, 0x42);
    pred_z6 = S(Scratchpad, 0x72);

    abs_val = (pred_x6 < 0) ? -pred_x6 : pred_x6;
    /* Penalty box X range: 0x2422 (37.000 m) .. 0x3320 (52.352 m) */
    if ((unsigned int)(abs_val - 0x2422) >= 0xEFFU) {
        return 0;
    }

    abs_val = (pred_z6 < 0) ? -pred_z6 : pred_z6;
    /* Penalty box half-width Z: < 0x12F5 (19.412 m) */
    if (abs_val >= 0x12F5) {
        return 0;
    }

    opp_id = B(Scratchpad, 0x31E + ((B(actor, 0x399) ^ 1) << 1));
    opp = D_800FF748 + (opp_id % 24U) * 1000;

    ball_x0 = S(Scratchpad, 0x36);
    ball_z0 = S(Scratchpad, 0x66);

    dx = S(actor, 2) - ball_x0;
    dz = S(actor, 6) - ball_z0;
    dist_self = dx * dx + dz * dz;

    dx = S(opp, 2) - ball_x0;
    dz = S(opp, 6) - ball_z0;
    dist_opp = dx * dx + dz * dz;

    if (dist_opp < dist_self) {
        switch (B(opp, 0x396)) {
        case 0x0D:
            if (B(opp, 0x390) < 6U) {
                return 0;
            }
            break;
        case 0x1E:
            if (B(opp, 0x390) < 7U) {
                return 0;
            }
            break;
        case 0x1F:
        case 0x20:
        case 0x22:
        case 0x23:
        case 0x24:
        case 0x25:
            if (B(opp, 0x390) < 8U) {
                return 0;
            }
            break;
        case 0x21:
            if (B(opp, 0x390) < 2U) {
                return 0;
            }
            break;
        case 0x26:
        case 0x27:
            if (B(opp, 0x390) < 4U) {
                return 0;
            }
            break;
        case 0x29:
            if (B(opp, 0x390) < 5U) {
                return 0;
            }
            break;
        default:
            break;
        }
    }

    dx = S(actor, 2) - pred_x6;
    dz = S(actor, 6) - pred_z6;
    dist_pred6 = (unsigned int)(dx * dx + dz * dz);

    pred_x7 = S(Scratchpad, 0x44);
    pred_z7 = S(Scratchpad, 0x74);
    dx = S(actor, 2) - pred_x7;
    dz = S(actor, 6) - pred_z7;
    dist_pred7 = (unsigned int)(dx * dx + dz * dz);

    if (dist_pred6 > 0x9000U && dist_pred7 < dist_pred6) {
        int abs_x7;
        int abs_self_x;
        int ok;

        target_ang = func_8005B36C(S(actor, 2), S(actor, 6), pred_x7, pred_z7);
        abs_x7 = (pred_x7 < 0) ? -pred_x7 : pred_x7;
        abs_self_x = (S(actor, 2) < 0) ? -S(actor, 2) : S(actor, 2);
        if (abs_x7 < abs_self_x && abs_x7 < 0x32A0) {
            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            diff = (target_ang - B(ball, 0x3A4)) & 0xFF;
            if ((unsigned int)diff < 0x81U) {
                ok = (diff < 0x40);
            } else {
                ok = (((B(ball, 0x3A4) - target_ang) & 0xFF) < 0x40);
            }
            if (!ok) {
                return 0;
            }
        }
    }

    target_ang = func_8005B36C(S(actor, 2), S(actor, 6), pred_x6, pred_z6);
    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    diff = (B(actor, 0x3AA) - B(ball, 0x3A4)) & 0xFF;
    if ((unsigned int)diff < 0x81U) {
        aligned = (diff < 0x21);
    } else {
        aligned = (((B(ball, 0x3A4) - B(actor, 0x3AA)) & 0xFF) < 0x21);
    }

    stat = ((W(D_8010C330, B(actor, 0x392) * 12 + B(actor, 0x398) * 264) >> 11) & 0x7F) + 0x64;
    reach_num = stat * 240;
    reach_div = (reach_num / 180) * reach_num;
    reach_sq = (unsigned int)(reach_div / 180);
    if (aligned) {
        int spd = SW(actor, 0x3A0);
        reach_sq = (unsigned int)((reach_div / 180) + (spd * spd * 36 + 0x4000));
    }

    stat = ((W(D_8010C330, B(actor, 0x392) * 12 + B(actor, 0x398) * 264) >> 11) & 0x7F) + 0x64;
    pred_y6 = S(Scratchpad, 0x5A);
    height_thresh = (-176 * stat) / 180;

    if (height_thresh < pred_y6) {
        /* Low ball */
        if (dist_pred6 < reach_sq || B(actor, 0x38D) == B(Scratchpad, 0x2A8)) {
            min_timer = 21 - ((B(actor, 0x3B9) / 10U) & 0xFF);
            if (B(Scratchpad, 0x2A9) == 0 && S(Scratchpad, 0x31C) < min_timer) {
                return 0;
            }
            func_800956C0(actor, min_timer);
            B(actor, 0x3AC) = 0;
            func_8005C2AC(actor, 0x60, 6, B(actor, 0x3AB), 0);
            SW(actor, 0x8) = (S(Scratchpad, 0x42) - S(Scratchpad, 0xF0)) / 7;
            SW(actor, 0x10) = (S(Scratchpad, 0x72) - S(Scratchpad, 0xF4)) / 7;
            H(actor, 0x3CC) = 8;
            S(actor, 0x4) = 0;
            W(actor, 0x3A0) = 0;
            B(actor, 0x3B0) = 0;
            B(actor, 0x3D1) = 0;
            B(actor, 0x396) = 0x88;
            B(actor, 0x397) = 0;
            return 1;
        }

        reach_num = stat * 512;
        reach_sq = (unsigned int)(((reach_num / 180) * reach_num) / 180);
        if (reach_sq < dist_pred6) {
            return 0;
        }
        if (B(Scratchpad, 0x2A9) == 0 && S(Scratchpad, 0x31C) < 10) {
            return 0;
        }

        diff = B(ball, 0x3A4) - target_ang;
        if ((unsigned int)(diff & 0xFF) >= 0x81U) {
            diff = target_ang - B(ball, 0x3A4);
        }
        if ((unsigned int)(((diff & 0xFF) - 0x24) & 0xFF) >= 0x29U) {
            return 0;
        }

        B(actor, 0x3D1) = 0;
        B(actor, 0x3B0) = 5;
        ang_to_ball0 = func_8005B36C(S(actor, 2), S(actor, 6), S(Scratchpad, 0x36), S(Scratchpad, 0x66));
        B(actor, 0x3AC) = (((target_ang - ang_to_ball0) & 0xFF) < 0x81U) ? 1 : 0;
        H(actor, 0x3C2) = H(Scratchpad, 0x42);
        H(actor, 0x3C4) = H(Scratchpad, 0x5A);
        H(actor, 0x3C6) = H(Scratchpad, 0x72);
        if (B(actor, 0x3AC) != 0) {
            new_heading = (unsigned char)((target_ang & 0xFF) - 0x40);
        } else {
            new_heading = (unsigned char)((target_ang & 0xFF) + 0x40);
        }
        B(actor, 0x3AB) = new_heading;
        B(actor, 0x396) = 0x8A;
        B(actor, 0x397) = 0;
        return 1;
    }

    height_thresh = (-480 * stat) / 180;
    if (height_thresh < pred_y6) {
        /* Medium-height ball */
        if (dist_pred6 < reach_sq) {
            min_timer = 21 - ((B(actor, 0x3B9) / 10U) & 0xFF);
            if (B(Scratchpad, 0x2A9) == 0 && S(Scratchpad, 0x31C) < min_timer) {
                return 0;
            }
            func_800956C0(actor, min_timer);
            if (W(Scratchpad, 0x2B0) == 4U && W(ball, 0x3A0) > 0x9000U) {
                func_8005C2AC(actor, 0x73, 6, B(actor, 0x3AB), B(actor, 0x3AC));
                B(actor, 0x396) = 0x99;
                B(actor, 0x397) = 0;
                vx = (S(Scratchpad, 0x42) - S(Scratchpad, 0xF0)) / 7;
                SW(actor, 0x8) = vx;
                vz = (S(Scratchpad, 0x72) - S(Scratchpad, 0xF4)) / 7;
                SW(actor, 0x10) = vz;
                S(actor, 0x4) = 0;
                /* Shift position by division remainder so 7 steps land on exact pred_x6/pred_z6 */
                H(actor, 0x2) += (unsigned short)(S(Scratchpad, 0x42) - (S(Scratchpad, 0xF0) + vx * 7));
                B(actor, 0x3AC) = 0;
                W(actor, 0x3A0) = 0;
                B(actor, 0x3B0) = 0;
                H(actor, 0x6) += (unsigned short)(S(Scratchpad, 0x72) - (S(Scratchpad, 0xF4) + vz * 7));
                B(actor, 0x3D1) = 0;
                return 1;
            }
            func_8005C2AC(actor, 0x5F, 6, B(actor, 0x3AB), B(actor, 0x3AC));
            SW(actor, 0x8) = (S(Scratchpad, 0x42) - S(Scratchpad, 0xF0)) / 7;
            S(actor, 0x4) = 0;
            B(actor, 0x397) = 0;
            B(actor, 0x396) = 0x87;
            SW(actor, 0x10) = (S(Scratchpad, 0x72) - S(Scratchpad, 0xF4)) / 7;
            B(actor, 0x3D1) = 0;
            return 1;
        }

        reach_num = stat * 512;
        reach_sq = (unsigned int)(((reach_num / 180) * reach_num) / 180);
        if (dist_pred6 >= reach_sq) {
            return 0;
        }
        if (B(Scratchpad, 0x2A9) == 0) {
            if (((-432 * stat) / 180) < pred_y6) {
                if (S(Scratchpad, 0x31C) < 10) {
                    return 0;
                }
            } else {
                if (S(Scratchpad, 0x31C) < 4) {
                    return 0;
                }
            }
        }

        diff = B(ball, 0x3A4) - target_ang;
        if ((unsigned int)(diff & 0xFF) >= 0x81U) {
            diff = target_ang - B(ball, 0x3A4);
        }
        if ((unsigned int)(((diff & 0xFF) - 0x24) & 0xFF) >= 0x29U) {
            return 0;
        }

        B(actor, 0x3B0) = 0;
        B(actor, 0x3D1) = 0;
        H(actor, 0x3C2) = H(Scratchpad, 0x42);
        H(actor, 0x3C4) = H(Scratchpad, 0x5A);
        H(actor, 0x3C6) = H(Scratchpad, 0x72);
        ang_to_ball0 = func_8005B36C(S(actor, 2), S(actor, 6), S(Scratchpad, 0x36), S(Scratchpad, 0x66));
        B(actor, 0x3AC) = (((target_ang - ang_to_ball0) & 0xFF) < 0x81U) ? 1 : 0;
        if (B(actor, 0x3AC) != 0) {
            new_heading = (unsigned char)((target_ang & 0xFF) - 0x40);
        } else {
            new_heading = (unsigned char)((target_ang & 0xFF) + 0x40);
        }
        B(actor, 0x3AB) = new_heading;
        stat = ((W(D_8010C330, B(actor, 0x392) * 12 + B(actor, 0x398) * 264) >> 11) & 0x7F) + 0x64;
        if (((-432 * stat) / 180) < S(Scratchpad, 0x5A)) {
            B(actor, 0x396) = 0x8A;
        } else {
            B(actor, 0x396) = 0xA1;
        }
        B(actor, 0x397) = 0;
        return 1;
    }

    /* High ball */
    if (pred_y6 < ((-608 * stat) / 180)) {
        return 0;
    }

    reach_num = stat * 240;
    reach_sq = (unsigned int)(((reach_num / 180) * reach_num) / 180);
    if (dist_pred6 < reach_sq) {
        min_timer = 19 - ((B(actor, 0x3B9) / 10U) & 0xFF);
        if (B(Scratchpad, 0x2A9) == 0 && S(Scratchpad, 0x31C) < min_timer) {
            return 0;
        }
        B(actor, 0x396) = 0x9F;
        B(actor, 0x397) = 0;
        B(actor, 0x3D1) = 0;
        return 1;
    }

    reach_num = stat * 512;
    reach_sq = (unsigned int)(((reach_num / 180) * reach_num) / 180);
    if (dist_pred6 >= reach_sq) {
        return 0;
    }

    diff = B(ball, 0x3A4) - target_ang;
    if ((unsigned int)(diff & 0xFF) >= 0x81U) {
        diff = target_ang - B(ball, 0x3A4);
    }
    diff &= 0xFF;
    if ((unsigned int)diff >= 0x4DU) {
        return 0;
    }

    if ((unsigned int)diff < 0x24U) {
        if (dist_pred7 < dist_pred6) {
            return 0;
        }
        min_timer = 19 - ((B(actor, 0x3B9) / 10U) & 0xFF);
        if (B(Scratchpad, 0x2A9) == 0 && S(Scratchpad, 0x31C) < min_timer) {
            return 0;
        }
        B(actor, 0x3D1) = 0;
        B(actor, 0x396) = 0x9F;
        B(actor, 0x397) = 0;
        return 1;
    }

    if (S(Scratchpad, 0x31C) < 4) {
        return 0;
    }
    H(actor, 0x3C2) = H(Scratchpad, 0x42);
    H(actor, 0x3C4) = H(Scratchpad, 0x5A);
    H(actor, 0x3C6) = H(Scratchpad, 0x72);
    ang_to_ball0 = func_8005B36C(S(actor, 2), S(actor, 6), S(Scratchpad, 0x36), S(Scratchpad, 0x66));
    B(actor, 0x3AC) = (((target_ang - ang_to_ball0) & 0xFF) < 0x81U) ? 1 : 0;
    if (B(actor, 0x3AC) != 0) {
        new_heading = (unsigned char)((target_ang & 0xFF) - 0x40);
    } else {
        new_heading = (unsigned char)((target_ang & 0xFF) + 0x40);
    }
    B(actor, 0x3AB) = new_heading;
    B(actor, 0x396) = 0xA1;
    B(actor, 0x397) = 0;
    B(actor, 0x3D1) = 0;
    return 1;
}
