/*
 * func_80031E80
 *
 * Purpose:
 *   Evaluates the 10 outfield teammates to select the best pass target for an
 *   actor, determines the pass type (+0x3C4: 0=ground, 1=through ball, 2=lofted/long,
 *   3=none) and target X coordinate (+0x3C8) with optional forward lead, or
 *   triggers an off-ball run (state 0x1B/0x40) on a close forward teammate.
 *
 * Arguments:
 *   actor   (unsigned char *): passing player's 1000-byte actor struct
 *   heading (unsigned char):   desired pass direction (0..255 = 360 deg)
 *
 * Return value:
 *   Actor ID (u8, +0x38D) of the chosen pass target, or 0 if none found
 *   (+0x3C4 == 3).
 *
 * Scratchpad / Actor fields read or written:
 *   Scratch reads:  0x2F4 (ptr to ball actor)
 *   Actor reads:    +0x02 (x), +0x06 (z), +0x38D (id), +0x399 (attack dir),
 *                   +0x39A (preset target flag), +0x3AA (body heading),
 *                   +0x3C2 (preset target id), +0x3C6 (desired distance scale)
 *   Actor writes:   +0x3C4 (pass type), +0x3C8 (target x)
 *   Teammate writes:+0x3AB, +0x396=0x1B, +0x397=0x40 (when close & ahead)
 *
 * Magic numbers / units (250 raw = 1 m, 256 angle units = 360 deg):
 *   0x30  = 48/256 (67.5 deg) max angle from pass heading
 *   0x40  = 64/256 (90 deg) max angle from body heading
 *   0x28  = 40/256 (56.25 deg) tight body-angle window
 *   0x10  = 16/256 (22.5 deg) max lead-angle deviation
 *   0x600 = 1536 raw (6.144 m), 0xC00 = 3072 raw (12.288 m), 0x1000 = 4096 raw (16.384 m)
 *   0x1400= 5120 raw (20.48 m), 0x2000 = 8192 raw (32.768 m), 0x23E2 = 9186 raw (36.744 m)
 *   0x24E2= 9442 raw (37.768 m), 0x1DE3 = 7651 raw (30.604 m)
 *   0x18FFFFF = 26214399 (~5120^2 = (20.48 m)^2), 0x240000 = 1536^2 ((6.144 m)^2), 0x400000 = 2048^2 ((8.192 m)^2)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define SCRATCH ((unsigned char*)0x1F800000)

extern unsigned char D_800FF748[];

unsigned int func_8005CE14(unsigned char actor_id);
unsigned int func_8005B36C(int x1, int z1, int x2, int z2);
unsigned int func_800B1128(unsigned int val);
unsigned int func_80052C6C(unsigned char *actor);
unsigned int func_8001E6B8(unsigned int n);
unsigned int func_800495B8(unsigned char *actor);

unsigned char func_80031E80(unsigned char *actor, unsigned char heading)
{
    unsigned int best_score = 0xFFFFFFFFu;
    unsigned char *mate_base;
    unsigned char *mate;
    unsigned char *best_target;
    unsigned int desired_dist;
    int dir_sign;
    int i;
    unsigned int fwd_diff = 0;

    mate_base = D_800FF748 + (func_8005CE14(B(actor, 0x38D)) & 0xFF) * 0x3E8 + 0x3E8;
    S(actor, 0x3C4) = 3;

    if (B(actor, 0x39A) != 0) {
        int idx = S(actor, 0x3C2) % 24;
        int dx, dz;
        best_target = D_800FF748 + idx * 0x3E8;
        heading = (unsigned char)func_8005B36C(S(actor, 2), S(actor, 6), S(best_target, 2), S(best_target, 6));
        dx = S(actor, 2) - S(best_target, 2);
        dz = S(actor, 6) - S(best_target, 6);
        desired_dist = func_800B1128((unsigned int)(dx * dx + dz * dz));
    } else {
        desired_dist = (unsigned int)(S(actor, 0x3C6) * 640);
    }

    dir_sign = (B(actor, 0x399) != 0) ? -1 : 1;

    mate = mate_base;
    for (i = 0; i < 10; i++) {
        unsigned int ang_to_mate;
        unsigned int ang_diff;
        unsigned int body_diff_raw;
        int body_ok;
        unsigned char *ball;
        int bdx, bdz;
        unsigned int ball_dist;
        int signed_x;
        unsigned int attack_heading;
        unsigned int fwd_raw;
        unsigned int cur_fwd_diff;
        int dist_err;
        int pass_type;
        unsigned int score;

        if (actor == mate) {
            goto next_mate;
        }
        if ((func_80052C6C(mate) & 0xFF) != 0) {
            goto next_mate;
        }

        ang_to_mate = func_8005B36C(S(actor, 2), S(actor, 6), S(mate, 2), S(mate, 6));
        ang_diff = ang_to_mate - heading;
        if ((ang_diff & 0xFF) >= 0x81) {
            ang_diff = heading - ang_to_mate;
        }
        if ((ang_diff & 0xFF) >= 0x31) {
            goto next_mate;
        }

        body_diff_raw = (ang_to_mate - B(actor, 0x3AA)) & 0xFF;
        if (body_diff_raw < 0x81) {
            body_ok = (int)body_diff_raw < 0x40;
        } else {
            body_ok = (int)((B(actor, 0x3AA) - ang_to_mate) & 0xFF) < 0x40;
        }
        if (!body_ok) {
            goto next_mate;
        }

        ball = (unsigned char *)W(SCRATCH, 0x2F4);
        bdx = S(ball, 2) - S(mate, 2);
        bdz = S(ball, 6) - S(mate, 6);
        ball_dist = func_800B1128((unsigned int)(bdx * bdx + bdz * bdz));

        signed_x = dir_sign * S(mate, 2);
        attack_heading = (unsigned int)B(actor, 0x399) << 7;
        fwd_raw = attack_heading - ang_to_mate;
        cur_fwd_diff = ((fwd_raw & 0xFF) < 0x81) ? fwd_raw : (ang_to_mate - attack_heading);
        fwd_diff = cur_fwd_diff;

        if ((cur_fwd_diff & 0xFF) < 0x30 && ball_dist < 0xC01) {
            B(mate, 0x3AB) = heading;
            B(mate, 0x396) = 0x1B;
            B(mate, 0x397) = 0x40;
            goto next_mate;
        }

        dist_err = (int)desired_dist - (int)ball_dist;
        if (dist_err < 0) {
            dist_err = -dist_err;
        }

        if (((unsigned short)(signed_x - 1)) < 0x23E1 && (cur_fwd_diff & 0xFF) < 0x39) {
            if (ball_dist < 0x2000 || func_8001E6B8(2) != 0) {
                if ((unsigned int)dist_err < 0x601) {
                    pass_type = 2;
                    goto score_candidate;
                }
                if (ball_dist >= 0x600) {
                    goto skip_mate_inc;
                }
                B(mate, 0x3AB) = heading;
                B(mate, 0x396) = 0x1B;
                B(mate, 0x397) = 0x40;
                goto next_mate;
            }
        }

        if (ball_dist < 0x600) {
            B(mate, 0x3AB) = heading;
            B(mate, 0x396) = 0x1B;
            B(mate, 0x397) = 0x40;
            goto next_mate;
        }

        body_diff_raw = (ang_to_mate - B(actor, 0x3AA)) & 0xFF;
        if (body_diff_raw < 0x81) {
            body_ok = (int)body_diff_raw < 0x28;
        } else {
            body_ok = (int)((B(actor, 0x3AA) - ang_to_mate) & 0xFF) < 0x28;
        }
        if (!body_ok || ball_dist < 0x1000 || (unsigned int)dist_err >= 0xA01) {
            goto next_mate;
        }

        pass_type = 0;
        if (((cur_fwd_diff - 0x28) & 0xFF) < 0x28) {
            pass_type = ((signed short)signed_x >= -0x7FF) ? 1 : 0;
        }

score_candidate:
        score = ((ang_diff & 0xFF) + 1) * (unsigned int)dist_err;
        if (score < best_score) {
            best_target = mate;
            S(actor, 0x3C4) = (signed short)pass_type;
            best_score = score;
        }

next_mate:
        ;
skip_mate_inc:
        mate += 0x3E8;
    }

    switch (S(actor, 0x3C4)) {
    case 3:
        return 0;

    case 1: {
        int dx = S(actor, 2) - S(best_target, 2);
        int dz = S(actor, 6) - S(best_target, 6);
        int dist_sq = dx * dx + dz * dz;
        unsigned int clear_sq = 0x240000;
        unsigned int lead = 0;
        unsigned short target_x;

        S(actor, 0x3C4) = 1;
        if (dist_sq > 0x18FFFFF) {
            if (B(actor, 0x399) == 0) {
                if (S(best_target, 2) < 0x1DE3) {
                    clear_sq = func_800495B8(best_target);
                }
            } else {
                if (-S(best_target, 2) < 0x1DE3) {
                    clear_sq = func_800495B8(best_target);
                }
            }
        }

        if (clear_sq > 0x400000 && (fwd_diff & 0xFF) < 0x51) {
            int abs_x;
            unsigned int step_pos = 0;
            int step_neg = 0;

            lead = (func_800B1128(clear_sq) - 0x400) >> 1;
            if (lead >= 0x801) {
                lead = 0x800;
            }
            abs_x = S(best_target, 2);
            if (abs_x < 0) {
                abs_x = -abs_x;
            }
            if ((unsigned int)(abs_x + (int)lead) >= 0x24E3) {
                if (abs_x < 0x23E2) {
                    lead = (unsigned int)(0x24E2 - abs_x);
                } else {
                    lead = 0;
                }
            }

            if (lead != 0) {
                do {
                    unsigned int base_ang = func_8005B36C(S(actor, 2), S(actor, 6), S(best_target, 2), S(best_target, 6));
                    int test_x = S(best_target, 2) + (B(actor, 0x399) ? step_neg : (int)step_pos);
                    unsigned int lead_ang = func_8005B36C(S(actor, 2), S(actor, 6), (signed short)test_x, S(best_target, 6));
                    int delta;
                    if (((lead_ang - base_ang) & 0xFF) < 0x81) {
                        delta = (int)((lead_ang - base_ang) & 0xFF);
                    } else {
                        delta = (int)((base_ang - lead_ang) & 0xFF);
                    }
                    if (delta >= 0x11) {
                        break;
                    }
                    step_pos += 0x200;
                    step_neg -= 0x200;
                } while (step_pos < lead);
            }
            lead = step_pos;
        }

        if (B(actor, 0x399) != 0) {
            target_x = (unsigned short)(S(best_target, 2) - (int)lead);
        } else {
            target_x = (unsigned short)(S(best_target, 2) + (int)lead);
        }
        S(actor, 0x3C8) = (signed short)target_x;
        return B(best_target, 0x38D);
    }

    case 2: {
        int dx = S(actor, 2) - S(best_target, 2);
        int dz = S(actor, 6) - S(best_target, 6);
        unsigned int dist = func_800B1128((unsigned int)(dx * dx + dz * dz));
        unsigned int lead = (dist * 8) / 20 + 0x600;
        unsigned short target_x;

        if (lead >= 0x1401) {
            lead = 0x1400;
        }
        if (B(actor, 0x399) != 0) {
            target_x = (unsigned short)(S(best_target, 2) - (int)lead);
        } else {
            target_x = (unsigned short)(S(best_target, 2) + (int)lead);
        }
        S(actor, 0x3C8) = (signed short)target_x;
        return B(best_target, 0x38D);
    }

    default:
        S(actor, 0x3C4) = 0;
        S(actor, 0x3C8) = S(best_target, 2);
        return B(best_target, 0x38D);
    }
}
