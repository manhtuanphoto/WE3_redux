/*
 * func_8006CA58
 *
 * Purpose:
 *   Goalkeeper (GK) loose-ball sweep / rush-out decision handler. Called when
 *   the ball is unowned (Scratchpad +0x2A9 == 0) and the GK is not already
 *   locked onto a target. Evaluates ball timer, ball event code, predicted
 *   ball intercept vs. penalty/goal-area bounds, and race distances against the
 *   closest teammate and opponent to decide whether to rush out and claim the
 *   ball (writing Scratchpad +0xF8/+0xFC, actor +0x3D4/+0x3B2/+0x3D0, or
 *   ball +0x3DC).
 *
 * Arguments:
 *   actor - pointer to 1000-byte GK actor struct
 *
 * Return value:
 *   None (void)
 *
 * Actor fields read:
 *   +0x02 (s16) x, +0x06 (s16) z, +0x38D (u8) actor id,
 *   +0x392 (u8) team slot, +0x396 (u8) state, +0x398 (u8) side (0/1),
 *   +0x399 (u8) attack direction flag, +0x3B4 (u8) max speed,
 *   +0x3B9 (u8) GK reaction/stat, +0x3BC (s16) ball-reach/intercept flag
 *
 * Actor fields written:
 *   +0x3B2 (u8) GK sweep/action flag, +0x3D0 (u8) action lock flag,
 *   +0x3D4 (u16) claim/mask flag (copied from Scratchpad +0x32[side])
 *
 * Opponent/teammate actor fields read (via D_800FF748 + (id % 24) * 1000):
 *   +0x02 (s16) x, +0x06 (s16) z, +0x399 (u8) attack direction flag,
 *   +0x3B4 (u8) max speed
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x32[side*2] (u16) team claim mask, +0x4C (s16) predicted ball x,
 *   +0x66 (s16) predicted ball z, +0xAC (u32),
 *   +0xF8/+0xFC (s16) predicted target x/z,
 *   +0x2AA (u8) previous ball owner, +0x2B0 (u32) ball event code,
 *   +0x2B4 (s16) ball trajectory sentinel (0x7FFF = unset/airborne),
 *   +0x2CF[side] (u8) possession flags, +0x2F4 (ptr) ball actor,
 *   +0x31C (s16) ball timer (frames since last touch),
 *   +0x31E[dir*2] (u8) closest actor id per direction
 *
 * Scratchpad / ball fields written:
 *   Scratchpad +0xF2 (s16) height threshold for intercept prediction,
 *   Scratchpad +0xF8/+0xFC (s16) target x/z,
 *   Ball +0x02/+0x06 (s16) read, Ball +0x3A4 (u8) move heading read,
 *   Ball +0x3DC (s16) GK rush/claim state (1 or 2)
 *
 * External data read:
 *   D_800C8524 (s16[]), D_800FF748 (actor array),
 *   D_8010C328 / D_8010C32F / D_8010C330 (player stat/role tables)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x17 / 0x1C / 0x21 (23 / 28 / 33 frames) -> 0.77 s / 0.93 s / 1.10 s
 *   0x30 (48 angle units) -> 67.5 deg; 0x31 (49) -> 68.9 deg; 0x70 (112) -> 157.5 deg
 *   -0x24E1 (-9441) -> -37.764 m x-coordinate (penalty box depth)
 *   -0x2462 (-9314) -> -37.256 m x-coordinate (penalty box depth)
 *   0x12F5 (4853) / 0x12F4 (4852) -> 19.412 m / 19.408 m z (penalty box half-width)
 *   0x400 (1024) -> 4.096 m distance
 *   0x100000 (1048576 = 1024^2) -> 4.096 m radius squared
 *   0x23FFFFF (~4859^2) -> ~19.44 m radius squared
 *   -0x2E08 (-11784) -> -47.136 m x-coordinate (near goal line)
 *   0x3B2 (946) -> 3.784 m z (goal half-width)
 *   -0x240 (-576) -> -2.304 m height (upward, ~crossbar height)
 *   0x10000 (65536 = 256^2) -> 1.024 m^2 margin; 0x40000 (512^2) -> 2.048 m^2 margin
 *   0x4000 (16384 = 128^2) -> 0.512 m^2 penalty; 0x6B6 (1718) -> 6.872 m z
 *   0x3321 (13089) / 0x3320 (13088) -> 52.356 m / 52.352 m x (goal line)
 *   0x8B6 (2230) -> 8.92 m z
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern signed short D_800C8524[];
extern unsigned char D_800FF748[];
extern unsigned char D_8010C328[];
extern unsigned char D_8010C32F[];
extern unsigned char D_8010C330[];

extern void func_800484B8(int x, int z, unsigned int max_speed);
extern void func_800484F8(int x, int z, unsigned int speed, int mode);
extern unsigned int func_800489A8(void);
extern unsigned int func_800495B8(unsigned char *actor);
extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);
extern int func_8006DA9C(unsigned char *actor, unsigned char *opp, int pred_x, unsigned int dist_sq);
extern unsigned int func_800B1128(unsigned int val);

void func_8006CA58(unsigned char *actor)
{
    unsigned char *scratch = Scratchpad;
    unsigned char *ball;
    int ball_dx;
    int ball_dz;
    unsigned int ball_dist_sq;
    int stat_row;
    int height_thresh;
    int mode;
    signed short pred_x;
    signed short pred_z;
    int adj_x;
    int abs_z;
    unsigned int pred_dist;
    unsigned char intercept_frames;
    int frames_needed;
    unsigned char atk_dir;
    unsigned char *opp;
    unsigned char *mate;
    int ix;
    int iz;
    int dx;
    int dz;
    unsigned int actor_dist_sq;
    unsigned int opp_dist_sq;
    unsigned int mate_dist_sq;
    int opp_reach;
    int idx;
    int x_thresh;
    int in_box;
    unsigned int bonus_margin;
    int goal_x;
    unsigned int ang1;
    unsigned int ang2;

    B(actor, 0x3B2) = 0;

    if ((int)S(scratch, 0x31C) < 23 - (int)(B(actor, 0x3B9) / 10)) {
        return;
    }

    switch (W(scratch, 0x2B0)) {
    case 1:
    case 3:
        if ((int)S(scratch, 0x31C) < 33 - (int)(B(actor, 0x3B9) / 10)) {
            return;
        }
        break;

    case 2:
        if (B(actor, 0x38D) == B(scratch, 0x2AA)) {
            return;
        }
        ball = (unsigned char *)W(scratch, 0x2F4);
        {
            unsigned int dir_ang = (unsigned int)B(actor, 0x399) << 7;
            unsigned int ball_ang = B(ball, 0x3A4);
            unsigned int diff = (dir_ang - ball_ang) & 0xFF;

            if (diff < 0x81U) {
                if ((int)diff < 0x30) {
                    return;
                }
            } else {
                if ((int)((ball_ang - dir_ang) & 0xFF) < 0x30) {
                    return;
                }
            }
        }
        break;

    case 4:
        if ((int)S(scratch, 0x31C) < 28 - (int)(B(actor, 0x3B9) / 10)) {
            return;
        }
        break;

    case 7:
        return;

    default:
        break;
    }

    ball = (unsigned char *)W(scratch, 0x2F4);
    ball_dx = S(actor, 0x02) - S(ball, 0x02);
    ball_dz = S(actor, 0x06) - S(ball, 0x06);
    ball_dist_sq = (unsigned int)(ball_dx * ball_dx + ball_dz * ball_dz);

    if (S(scratch, 0x2B4) != 0x7FFF) {
        func_800495B8(actor);

        stat_row = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
        if (W(scratch, 0x2B0) == 4 && S(actor, 0x3BC) != 0 && ball_dist_sq <= 0x23FFFFFU) {
            int stat = (int)((W(D_8010C330, stat_row) >> 11) & 0x7F) + 100;
            mode = 2;
            height_thresh = (-296 * stat) / 180;
        } else {
            int stat = (int)((W(D_8010C330, stat_row) >> 11) & 0x7F) + 100;
            int nibble = B(D_8010C32F, stat_row) & 0xF;
            mode = 3;
            height_thresh = ((-736 * stat) / 180) - nibble + 70;
        }

        S(scratch, 0x0F2) = (signed short)height_thresh;
        func_800484F8(S(actor, 0x02), S(actor, 0x06), (unsigned short)(B(actor, 0x3B4) - 1), mode);

        pred_x = S(scratch, 0x0F8);
        pred_z = S(scratch, 0x0FC);

        adj_x = (B(actor, 0x399) != 0) ? -pred_x : pred_x;
        if (adj_x >= -0x24E1) {
            return;
        }

        abs_z = (pred_z >= 0) ? pred_z : -pred_z;
        if (abs_z >= 0x12F5) {
            return;
        }

        dx = S(actor, 0x02) - pred_x;
        dz = S(actor, 0x06) - pred_z;
        pred_dist = func_800B1128((unsigned int)(dx * dx + dz * dz));
        if (pred_dist < 0x400U && ball_dist_sq <= 0x100000U) {
            H(actor, 0x3D4) = H(scratch, 0x32 + B(actor, 0x398) * 2);
            return;
        }

        ball = (unsigned char *)W(scratch, 0x2F4);
        /* UNCLEAR: In the MIPS binary, `negu $v0, $v0` is placed in the delay
         * slot of `beqz $v1, .L8006CE1C`, so `-S(ball, 0x02)` is compared
         * against `-0x2E08` regardless of whether `B(actor, 0x399)` is 0 or 1. */
        if (B(actor, 0x399) != 0) {
            if (-S(ball, 0x02) < -0x2E08) {
                int bz = S(ball, 0x06);
                if (((bz >= 0) ? bz : -bz) < 0x3B2) {
                    H(actor, 0x3D4) = H(scratch, 0x32 + B(actor, 0x398) * 2);
                    return;
                }
            }
        } else {
            if (-S(ball, 0x02) < -0x2E08) {
                int bz = S(ball, 0x06);
                if (((bz >= 0) ? bz : -bz) < 0x3B2) {
                    H(actor, 0x3D4) = H(scratch, 0x32 + B(actor, 0x398) * 2);
                    return;
                }
            }
        }

        S(scratch, 0x0F2) = -0x240;
        intercept_frames = (unsigned char)(func_800489A8() & 0xFF);
        ix = S(scratch, 0x0F8);
        iz = S(scratch, 0x0FC);
        dx = S(actor, 0x02) - ix;
        dz = S(actor, 0x06) - iz;
        pred_dist = func_800B1128((unsigned int)(dx * dx + dz * dz));
        frames_needed = (int)(pred_dist / (unsigned int)B(actor, 0x3B4)) + 2;

        if ((int)intercept_frames >= frames_needed) {
            H(actor, 0x3D4) = H(scratch, 0x32 + B(actor, 0x398) * 2);
            ball = (unsigned char *)W(scratch, 0x2F4);
            S(scratch, 0x0F8) = pred_x;
            S(scratch, 0x0FC) = pred_z;
            S(ball, 0x3DC) = 1;
            return;
        }

        if (W(scratch, 0x2B0) == 4 && S(actor, 0x3BC) != 0 && ball_dist_sq <= 0x23FFFFFU) {
            S(scratch, 0x0F8) = pred_x;
            S(scratch, 0x0FC) = pred_z;
            H(actor, 0x3D4) = H(scratch, 0x32 + B(actor, 0x398) * 2);
            return;
        }

        atk_dir = B(actor, 0x399);
        opp = D_800FF748 + (B(scratch, 0x31E + (atk_dir ^ 1) * 2) % 24) * 1000;
        actor_dist_sq = (unsigned int)((S(actor, 0x02) - ix) * (S(actor, 0x02) - ix) +
                                       (S(actor, 0x06) - iz) * (S(actor, 0x06) - iz));
        opp_dist_sq = (unsigned int)((S(opp, 0x02) - ix) * (S(opp, 0x02) - ix) +
                                     (S(opp, 0x06) - iz) * (S(opp, 0x06) - iz));
        opp_reach = (int)intercept_frames * (int)B(opp, 0x3B4) + 0x400;

        if (opp_dist_sq < (unsigned int)(opp_reach * opp_reach)) {
            mate = D_800FF748 + (B(scratch, 0x31E + atk_dir * 2) % 24) * 1000;
            mate_dist_sq = (unsigned int)((S(mate, 0x02) - ix) * (S(mate, 0x02) - ix) +
                                          (S(mate, 0x06) - iz) * (S(mate, 0x06) - iz));
            if ((int)mate_dist_sq < (int)(actor_dist_sq + 0x10000U)) {
                return;
            }
            if (func_8006DA9C(actor, opp, pred_x, actor_dist_sq) == 0) {
                return;
            }
            if (S(actor, 0x3BC) == 0) {
                return;
            }
            B(actor, 0x3B2) = 1;
            H(actor, 0x3D4) = H(scratch, 0x32 + B(actor, 0x398) * 2);
            return;
        }

        stat_row = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
        idx = W(D_8010C328, stat_row) & 0xF;
        x_thresh = D_800C8524[idx];
        if (((atk_dir != 0) ? (-ix < x_thresh) : (ix < x_thresh)) &&
            S(actor, 0x3BC) != 0) {
            int abs_iz = (iz >= 0) ? iz : -iz;
            if (abs_iz < 0x6B6) {
                int stat = (int)((W(D_8010C330, stat_row) >> 11) & 0x7F) + 100;
                int nibble = B(D_8010C32F, stat_row) & 0xF;
                height_thresh = ((-736 * stat) / 180) - nibble + 70;
                S(scratch, 0x0F2) = (signed short)height_thresh;
                func_800484F8(S(actor, 0x02), S(actor, 0x06), (unsigned short)(B(actor, 0x3B4) - 1), 0);
                H(actor, 0x3D4) = H(scratch, 0x32 + B(actor, 0x398) * 2);
                return;
            }
        }

        mate = D_800FF748 + (B(scratch, 0x31E + B(actor, 0x399) * 2) % 24) * 1000;
        actor_dist_sq = (unsigned int)((S(actor, 0x02) - ix) * (S(actor, 0x02) - ix) +
                                       (S(actor, 0x06) - iz) * (S(actor, 0x06) - iz));
        mate_dist_sq = (unsigned int)((S(mate, 0x02) - ix) * (S(mate, 0x02) - ix) +
                                      (S(mate, 0x06) - iz) * (S(mate, 0x06) - iz));
        if ((int)mate_dist_sq < (int)(actor_dist_sq + 0x40000U)) {
            return;
        }

        ball = (unsigned char *)W(scratch, 0x2F4);
        S(ball, 0x3DC) = 2;
        {
            int stat = (int)((W(D_8010C330, stat_row) >> 11) & 0x7F) + 100;
            int nibble = B(D_8010C32F, stat_row) & 0xF;
            height_thresh = ((-736 * stat) / 180) - nibble + 70;
        }
        S(scratch, 0x0F2) = (signed short)height_thresh;
        func_800484F8(S(actor, 0x02), S(actor, 0x06), (unsigned short)(B(actor, 0x3B4) - 1), 0);
        H(actor, 0x3D4) = H(scratch, 0x32 + B(actor, 0x398) * 2);
        return;
    }

    /* Sentinel S(scratch, 0x2B4) == 0x7FFF */
    stat_row = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
    if (W(scratch, 0x2B0) == 4 && S(actor, 0x3BC) != 0 && ball_dist_sq <= 0x23FFFFFU) {
        int stat = (int)((W(D_8010C330, stat_row) >> 11) & 0x7F) + 100;
        mode = 2;
        height_thresh = (-296 * stat) / 180;
    } else {
        int stat = (int)((W(D_8010C330, stat_row) >> 11) & 0x7F) + 100;
        int nibble = B(D_8010C32F, stat_row) & 0xF;
        mode = 3;
        height_thresh = ((-736 * stat) / 180) - nibble + 70;
    }

    S(scratch, 0x0F2) = (signed short)height_thresh;
    func_800484F8(S(actor, 0x02), S(actor, 0x06), B(actor, 0x3B4), mode);

    pred_x = S(scratch, 0x0F8);
    pred_z = S(scratch, 0x0FC);
    adj_x = (B(actor, 0x399) != 0) ? -pred_x : pred_x;
    abs_z = (pred_z >= 0) ? pred_z : -pred_z;
    if (adj_x < -0x2462 && abs_z < 0x12F4) {
        in_box = 1;
    } else {
        in_box = 0;
    }

    actor_dist_sq = (unsigned int)((S(actor, 0x02) - pred_x) * (S(actor, 0x02) - pred_x) +
                                   (S(actor, 0x06) - pred_z) * (S(actor, 0x06) - pred_z));

    if (B(actor, 0x396) != 0x9D &&
        B(scratch, 0x2CF + B(actor, 0x398)) == 1 &&
        S(actor, 0x3BC) == 0) {
        ball = (unsigned char *)W(scratch, 0x2F4);
        {
            unsigned int dir_ang = (unsigned int)B(actor, 0x399) << 7;
            unsigned int ball_ang = B(ball, 0x3A4);
            unsigned int diff = (dir_ang - ball_ang) & 0xFF;
            int far_angle = 0;

            if (diff < 0x81U) {
                if ((int)diff >= 0x70) {
                    far_angle = 1;
                }
            } else {
                if ((int)((ball_ang - dir_ang) & 0xFF) >= 0x70) {
                    far_angle = 1;
                }
            }

            if (far_angle) {
                int s66 = S(scratch, 0x066);
                int s4c = S(scratch, 0x04C);
                if (((s66 >= 0) ? s66 : -s66) >= 0x400) {
                    if (((s4c >= 0) ? s4c : -s4c) >= 0x3321 || W(scratch, 0x0AC) >= 0x4001U) {
                        B(actor, 0x3D0) = 1;
                        return;
                    }
                }
            }
        }
    }

    if (in_box != 0 && S(actor, 0x3BC) != 0 && W(scratch, 0x2B0) == 4) {
        H(actor, 0x3D4) = H(scratch, 0x32 + B(actor, 0x398) * 2);
        return;
    }

    atk_dir = B(actor, 0x399);
    mate = D_800FF748 + (B(scratch, 0x31E + atk_dir * 2) % 24) * 1000;
    mate_dist_sq = (unsigned int)((S(mate, 0x02) - pred_x) * (S(mate, 0x02) - pred_x) +
                                  (S(mate, 0x06) - pred_z) * (S(mate, 0x06) - pred_z));
    bonus_margin = (B(actor, 0x396) != 0x9D) ? (1U << 14) : 0U;
    if (mate_dist_sq + bonus_margin < actor_dist_sq) {
        return;
    }

    opp = D_800FF748 + (B(scratch, 0x31E + (atk_dir ^ 1) * 2) % 24) * 1000;
    func_800484B8(S(opp, 0x02), S(opp, 0x06), B(opp, 0x3B4));

    ix = S(scratch, 0x0F8);
    iz = S(scratch, 0x0FC);
    opp_dist_sq = (unsigned int)((S(opp, 0x02) - ix) * (S(opp, 0x02) - ix) +
                                 (S(opp, 0x06) - iz) * (S(opp, 0x06) - iz));

    stat_row = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
    idx = W(D_8010C328, stat_row) & 0xF;
    if (idx < 7) {
        int margin = (7 - idx) * 128;
        actor_dist_sq += (unsigned int)(margin * margin);
    }

    if (actor_dist_sq + bonus_margin < opp_dist_sq) {
        if (in_box == 0) {
            int stat = (int)((W(D_8010C330, stat_row) >> 11) & 0x7F) + 100;
            S(scratch, 0x0F2) = (signed short)((-296 * stat) / 180);
            func_800484F8(S(actor, 0x02), S(actor, 0x06), B(actor, 0x3B4), mode);
        } else {
            S(scratch, 0x0F8) = pred_x;
            S(scratch, 0x0FC) = pred_z;
        }
        H(actor, 0x3D4) = H(scratch, 0x32 + B(actor, 0x398) * 2);
        return;
    }

    if (in_box == 0) {
        return;
    }

    if (((iz >= 0) ? iz : -iz) >= 0x8B6) {
        return;
    }

    goal_x = (B(opp, 0x399) != 0) ? -0x3320 : 0x3320;
    ang1 = func_8005B36C(ix, iz, goal_x, 0);
    if (((ang1 - ((unsigned int)B(opp, 0x399) << 7)) & 0xFF) < 0x81U) {
        goal_x = (B(opp, 0x399) != 0) ? -0x3320 : 0x3320;
        ang2 = func_8005B36C(S(scratch, 0x0F8), S(scratch, 0x0FC), goal_x, 0);
        if ((int)((ang2 - ((unsigned int)B(opp, 0x399) << 7)) & 0xFF) >= 0x31) {
            return;
        }
    } else {
        goal_x = (B(opp, 0x399) != 0) ? -0x3320 : 0x3320;
        ang2 = func_8005B36C(S(scratch, 0x0F8), S(scratch, 0x0FC), goal_x, 0);
        if ((int)((((unsigned int)B(opp, 0x399) << 7) - ang2) & 0xFF) >= 0x31) {
            return;
        }
    }

    B(actor, 0x3B2) = 1;
    H(actor, 0x3D4) = H(scratch, 0x32 + B(actor, 0x398) * 2);
}
