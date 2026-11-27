/*
 * func_8005D28C
 *
 * Purpose:
 *   Updates an off-ball player's AI formation/positioning target (actor+0x3BC x,
 *   +0x3BE z), staggered so only one of the 22 players updates per frame
 *   (actor_id % 22 == frame_counter % 22). Computes base formation coordinates
 *   by role/mode, applies tactical offsets, offside/halfway clamping, attacking
 *   lane separation, and ball-avoidance push-out, then clamps to pitch bounds.
 *
 * Arguments:
 *   actor (a0) - pointer to 1000-byte player actor struct
 *
 * Return value:
 *   void
 *
 * Scratchpad (0x1F800000) fields read/written:
 *   0x290 (s32, R): frame counter (30 frames/s)
 *   0x2AE (u8, R): actor ID (used when actor+0x3E2 == 2)
 *   0x2A9 (u8, R): current ball owner (early return if actor owns ball)
 *   0x2B0 (u32, R): ball event code (3 checked with 0x2B8 != 0x7FFF)
 *   0x2B8 (s16, R): target/lock sentinel (0x7FFF)
 *   0x2CF (u8[2], R): per-side possession flags
 *   0x2F4 (u8 *, R): pointer to ball actor
 *   0x2FF (u8[2], R): per-side tactical mentality/line mode (0, 1, 2)
 *   0x30C (s16[2], R): per-dir defensive line X reference
 *   0x314, 0x318 (s32, R): offside / boundary X limits per attack dir
 *
 * Actor fields read/written:
 *   +0x002, +0x006 (s16, R): actor x, z position (250 raw = 1 m)
 *   +0x38D (u8, R): actor ID
 *   +0x391 (u8, R): tactical role/position index (0..25)
 *   +0x398 (u8, R): side (0/1)
 *   +0x399 (u8, R): attack direction flag (0 = +X, 1 = -X)
 *   +0x3BC, +0x3BE (s16, R/W): target formation x, z position
 *   +0x3D4 (u16, R/W): cached/restored state value
 *   +0x3D6 (u16, W): cleared to 0 on early returns
 *   +0x3D8 (u16, R/W): backup of +0x3D4
 *   +0x3DA (u8, R): positioning flags (bits 0..1 skip base update, bits 2..3 jitter dir)
 *   +0x3DC, +0x3DE (s16, R/W): x, z formation jitter offset
 *   +0x3E1 (u8, W): cleared to 0 on tactical hold
 *   +0x3E2 (u8, R): formation solver mode (1, 2, 3)
 *   +0x3E3 (u8, R): midfield-hold override flag
 *
 * Globals read:
 *   D_800C8488 (u8[32]): 8x4 direction table for jitter offsets
 *   D_800FF748 (u8[]): 1000-byte actor array
 *   D_8010C1D8 (u8[]): team tactic/formation data (side + 0xD checked == 6)
 *
 * Magic numbers & unit conversions (250 raw = 1 m, 30 frames/s):
 *   0x280 (640 raw = 2.56 m), 0x400 (1024 raw = 4.096 m): jitter offsets
 *   0x500 (1280 raw = 5.12 m), 0x700 (1792 raw = 7.168 m): tactical line shifts
 *   0x800 (2048 raw = 8.192 m): ball-lane Z separation
 *   0x9E8 (2536 raw = 10.144 m): ball-avoidance push-out radius (sq thresh 0x53DCA1 -> ~2344 raw = 9.38 m)
 *   0x110B (4363 raw = 17.452 m): ball X threshold for shot-lane avoidance
 *   0x2744 (10052 raw = 40.208 m), 0x28F5 (10485 raw = 41.94 m),
 *   0x2A08 (10760 raw = 43.04 m), 0x2E08 (11784 raw = 47.136 m): X clamps
 *   0x32F0 (13040 raw = 52.16 m), 0x20D0 (8400 raw = 33.6 m, thresh 0x2101 = 33.8 m): pitch X/Z clamps
 *   0x3320 (13088 raw = 52.352 m): goal line X; 0x63FFFFF (~10240^2 = 40.96^2 m^2): goal dist^2
 *   0x11 (17 = 23.9 deg), 0x20 (32 = 45 deg): angular thresholds
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))

#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800C8488[];
extern unsigned char D_800FF748[];
extern unsigned char D_8010C1D8[];

extern void func_800A5980(unsigned char *actor, unsigned char *tx, unsigned char *tz, int dir);
extern void func_800A3EC8(unsigned char *actor, unsigned char *tx, unsigned char *tz);
extern void func_800A4D30(unsigned char *actor, unsigned char *tx, unsigned char *tz);
extern int func_8005A76C(int cur, int target, int step);
extern void func_800A1484(unsigned char *actor, unsigned char *tx, unsigned char *tz, int param);
extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);
extern int func_8001E694(unsigned int angle, int scale);
extern int func_8001E5A4(unsigned int angle, int scale);

void func_8005D28C(unsigned char *actor)
{
    unsigned char actor_id;
    int dir_sign;
    unsigned short saved_3d4;
    unsigned char *opp;
    unsigned char *ball;
    unsigned char tactic;
    int limit;
    int dx, dz;
    int dist_sq;
    unsigned int ang;
    int tmp;

    actor_id = B(actor, 0x38D);
    if (actor_id == B(Scratchpad, 0x2A9)) {
        return;
    }

    if ((unsigned int)(actor_id % 22U) != (unsigned int)(SW(Scratchpad, 0x290) % 22)) {
        H(actor, 0x3D6) = 0;
        H(actor, 0x3D4) = H(actor, 0x3D8);
        return;
    }

    saved_3d4 = H(actor, 0x3D4);
    dir_sign = (B(actor, 0x399) != 0) ? -1 : 1;
    H(actor, 0x3D8) = saved_3d4;

    if (B(Scratchpad, 0x2CF + B(actor, 0x398)) == 0) {
        /* Team does not have possession */
        if (W(Scratchpad, 0x2B0) == 3U && S(Scratchpad, 0x2B8) != 0x7FFF) {
            H(actor, 0x3D4) = saved_3d4;
            H(actor, 0x3D6) = 0;
            return;
        }

        if ((B(actor, 0x3DA) & 3) == 0) {
            switch (B(actor, 0x3E2)) {
            case 1:
                func_800A4D30(actor, actor + 0x3BC, actor + 0x3BE);
                break;
            case 2:
                opp = D_800FF748 + B(Scratchpad, 0x2AE) * 1000;
                func_800A5980(actor, actor + 0x3BC, actor + 0x3BE, B(opp, 0x399));
                break;
            case 3:
                func_800A3EC8(actor, actor + 0x3BC, actor + 0x3BE);
                break;
            default:
                break;
            }

            switch (D_800C8488[((B(actor, 0x38D) & 7) << 2) + ((B(actor, 0x3DA) >> 2) & 3)]) {
            case 0:
                S(actor, 0x3DC) = 0x280;
                S(actor, 0x3DE) = 0;
                break;
            case 1:
                S(actor, 0x3DC) = 0;
                S(actor, 0x3DE) = 0x400;
                break;
            case 2:
                S(actor, 0x3DC) = -0x280;
                S(actor, 0x3DE) = 0;
                break;
            case 3:
                S(actor, 0x3DC) = 0;
                S(actor, 0x3DE) = -0x400;
                break;
            default:
                S(actor, 0x3DC) = 0;
                S(actor, 0x3DE) = 0x400;
                break;
            }

            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            if ((B(actor, 0x399) == 0 && S(ball, 2) < 0) ||
                (B(actor, 0x399) == 1 && S(ball, 2) > 0)) {
                S(actor, 0x3DE) = S(actor, 0x3DE) / 2;
            }

            switch (B(actor, 0x391)) {
            case 14:
            case 15:
            case 16:
                if ((B(actor, 0x399) == 0 && S(ball, 2) < 0) ||
                    (B(actor, 0x399) == 1 && S(ball, 2) > 0)) {
                    S(actor, 0x3DE) = 0;
                    S(actor, 0x3DC) = 0;
                }
                break;
            case 17:
            case 18:
            case 21:
            case 22:
            case 23:
            case 24:
            case 25:
                S(actor, 0x3DE) = 0;
                S(actor, 0x3DC) = 0;
                break;
            case 19:
            case 20:
            default:
                break;
            }

            if (B(actor, 0x3E2) == 1) {
                tactic = B(Scratchpad, 0x2FF + B(actor, 0x398));
                if (tactic == 1) {
                    S(actor, 0x3BC) = func_8005A76C(S(actor, 0x3BC), S(Scratchpad, 0x30C + B(actor, 0x399) * 2), 1);
                    S(actor, 0x3BE) = func_8005A76C(S(actor, 0x3BE), 0, 1);
                } else if (tactic == 2) {
                    S(actor, 0x3BC) = func_8005A76C(S(actor, 0x3BC), S(Scratchpad, 0x30C + B(actor, 0x399) * 2), 2);
                    S(actor, 0x3BE) = func_8005A76C(S(actor, 0x3BE), 0, 2);
                }
            } else if (B(actor, 0x3E2) != 0 && B(actor, 0x3E2) < 4) {
                tactic = B(Scratchpad, 0x2FF + B(actor, 0x398));
                if (tactic == 1) {
                    unsigned char role = B(actor, 0x391);
                    if (role != 0 && role < 6) {
                        S(actor, 0x3BC) += (signed short)(dir_sign * 0x400);
                        S(actor, 0x3BE) = func_8005A76C(S(actor, 0x3BE), 0, 1);
                    } else if (role == 19 || role == 20) {
                        S(actor, 0x3BC) = func_8005A76C(S(actor, 0x3BC), S(Scratchpad, 0x30C + B(actor, 0x399) * 2), 2);
                    } else {
                        S(actor, 0x3BC) = func_8005A76C(S(actor, 0x3BC), S(Scratchpad, 0x30C + B(actor, 0x399) * 2), 1);
                        S(actor, 0x3BE) = func_8005A76C(S(actor, 0x3BE), 0, 1);
                    }
                    tmp = S(actor, 0x3BC);
                    if (((tmp < 0) ? -tmp : tmp) >= 0x2745) {
                        S(actor, 0x3BC) = (tmp >= 0) ? 0x2744 : -0x2744;
                    }
                } else if (tactic == 2) {
                    switch (B(actor, 0x391)) {
                    case 1:
                    case 2:
                    case 3:
                    case 4:
                    case 5:
                        S(actor, 0x3BC) += (signed short)(dir_sign * 0x700);
                        S(actor, 0x3BE) = func_8005A76C(S(actor, 0x3BE), 0, 2);
                        break;
                    case 9:
                    case 10:
                    case 11:
                    case 12:
                    case 13:
                        S(actor, 0x3BC) = func_8005A76C(S(actor, 0x3BC), S(Scratchpad, 0x30C + B(actor, 0x399) * 2), 2);
                        /* fallthrough */
                    case 6:
                    case 7:
                    case 8:
                    case 14:
                    case 15:
                    case 16:
                    case 21:
                    case 22:
                    default:
                        S(actor, 0x3BC) = func_8005A76C(S(actor, 0x3BC), S(Scratchpad, 0x30C + B(actor, 0x399) * 2), 2);
                        S(actor, 0x3BE) = func_8005A76C(S(actor, 0x3BE), 0, 2);
                        break;
                    case 19:
                    case 20:
                        S(actor, 0x3BC) = func_8005A76C(S(actor, 0x3BC), S(Scratchpad, 0x30C + B(actor, 0x399) * 2), 3);
                        break;
                    case 17:
                    case 18:
                    case 23:
                    case 24:
                        break;
                    }
                    tmp = S(actor, 0x3BC);
                    if (((tmp < 0) ? -tmp : tmp) >= 0x2745) {
                        S(actor, 0x3BC) = (tmp >= 0) ? 0x2744 : -0x2744;
                    }
                } else {
                    tmp = S(actor, 0x3BC);
                    if (((tmp < 0) ? -tmp : tmp) >= 0x2745) {
                        S(actor, 0x3BC) = (tmp >= 0) ? 0x2744 : -0x2744;
                    }
                }
            }

            S(actor, 0x3BC) += S(actor, 0x3DC);
            S(actor, 0x3BE) += S(actor, 0x3DE);

            if (B(actor, 0x399) == 0) {
                limit = SW(Scratchpad, 0x318) - 0x100;
                if (S(actor, 0x3BC) > limit) {
                    S(actor, 0x3BC) = (signed short)limit;
                }
            }
            if (B(actor, 0x399) == 1) {
                limit = SW(Scratchpad, 0x314) + 0x100;
                if (S(actor, 0x3BC) < limit) {
                    S(actor, 0x3BC) = (signed short)limit;
                }
            }

            if (B(actor, 0x391) >= 6U) {
                tmp = S(actor, 0x3BC);
                if (((tmp < 0) ? -tmp : tmp) >= 0x2A09) {
                    S(actor, 0x3BC) = (tmp >= 0) ? 0x2A08 : -0x2A08;
                }
            }
            tmp = S(actor, 0x3BC);
            if (((tmp < 0) ? -tmp : tmp) >= 0x2E09) {
                S(actor, 0x3BC) = (tmp >= 0) ? 0x2E08 : -0x2E08;
            }
        }

        if (B(actor, 0x3E3) == 0) {
            unsigned char role = B(actor, 0x391);
            if ((role == 17 || role == 18 || role == 23 || role == 24) &&
                S(actor, 0x3BC) * dir_sign > 0) {
                S(actor, 0x3BC) = 0;
            }
        }
    } else {
        /* Team has possession */
        switch (B(actor, 0x3E2)) {
        case 1:
            func_800A4D30(actor, actor + 0x3BC, actor + 0x3BE);
            if (B(D_8010C1D8, B(actor, 0x398) + 0xD) == 6 && B(actor, 0x391) < 9U) {
                S(actor, 0x3BC) += (signed short)(dir_sign * 0x500);
            }
            break;
        case 2:
            opp = D_800FF748 + B(Scratchpad, 0x2AE) * 1000;
            func_800A5980(actor, actor + 0x3BC, actor + 0x3BE, B(opp, 0x399));
            tmp = S(actor, 0x3BC);
            if (((tmp < 0) ? -tmp : tmp) >= 0x28F6) {
                S(actor, 0x3BC) = (tmp >= 0) ? 0x28F5 : -0x28F5;
            }
            break;
        case 3:
            func_800A3EC8(actor, actor + 0x3BC, actor + 0x3BE);
            break;
        default:
            break;
        }

        switch (B(Scratchpad, 0x2FF + B(actor, 0x398))) {
        case 0:
            S(actor, 0x3BC) -= (signed short)(dir_sign * 0x500);
            break;
        case 1:
            S(actor, 0x3BC) -= (signed short)(dir_sign * 0x100);
            break;
        case 2:
            S(actor, 0x3BC) += (signed short)(dir_sign * 0x300);
            break;
        default:
            break;
        }

        switch (B(actor, 0x391)) {
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
            if (B(D_8010C1D8, B(actor, 0x398) + 0xD) == 6) {
                S(actor, 0x3BC) = func_8005A76C(S(actor, 0x3BC), SW(Scratchpad, 0x314 + (B(actor, 0x399) ^ 1) * 4), 5);
            }
            /* fallthrough */
        case 6:
        case 7:
        case 8:
        default:
            if (B(Scratchpad, 0x2FF + B(actor, 0x398)) == 0) {
                func_800A1484(actor, actor + 0x3BC, actor + 0x3BE, 0x1400);
            } else {
                B(actor, 0x3E1) = 0;
            }
            break;
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
            if (B(Scratchpad, 0x2FF + B(actor, 0x398)) < 2U) {
                func_800A1484(actor, actor + 0x3BC, actor + 0x3BE, 0x1200);
            } else {
                B(actor, 0x3E1) = 0;
            }
            break;
        case 14:
        case 15:
        case 16:
            func_800A1484(actor, actor + 0x3BC, actor + 0x3BE, 0x1000);
            break;
        case 17:
        case 18:
        case 19:
        case 20:
            func_800A1484(actor, actor + 0x3BC, actor + 0x3BE, 0xC00);
            break;
        case 21:
        case 22:
        case 23:
            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            if ((B(actor, 0x399) == 0 && S(ball, 2) < 0) ||
                (B(actor, 0x399) == 1 && S(ball, 2) > 0)) {
                func_800A1484(actor, actor + 0x3BC, actor + 0x3BE, 0xC00);
            }
            break;
        case 24:
        case 25:
            break;
        }

        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        if (dir_sign * S(ball, 2) < dir_sign * S(actor, 0x3BC)) {
            dz = S(actor, 0x3BE) - S(ball, 6);
            if (((dz < 0) ? -dz : dz) < 0x800) {
                S(actor, 0x3BE) += (dz >= 0) ? 0x800 : -0x800;
            }
        }
    }

    if (B(Scratchpad, 0x2CF + B(actor, 0x398)) == 0 && B(actor, 0x3E2) == 2) {
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        tmp = S(ball, 2);
        if (((tmp < 0) ? -tmp : tmp) >= 0x110B) {
            unsigned int goal_ang = B(actor, 0x399) << 7;
            unsigned char ball_face = B(ball, 0x3A8);
            int diff = (ball_face - goal_ang) & 0xFF;
            int facing_goal;

            if ((unsigned int)diff < 0x81U) {
                facing_goal = (diff < 0x20);
            } else {
                facing_goal = (((goal_ang - ball_face) & 0xFF) < 0x20);
            }

            if (facing_goal) {
                int goal_x = (B(actor, 0x399) != 0) ? -0x3320 : 0x3320;
                unsigned int ang_goal = func_8005B36C(S(ball, 2), S(ball, 6), goal_x, 0);
                unsigned int ang_tgt = func_8005B36C(S(ball, 2), S(ball, 6), S(actor, 0x3BC), S(actor, 0x3BE));
                int gx_off = (B(actor, 0x399) != 0) ? (S(ball, 2) + 0x3320) : (S(ball, 2) - 0x3320);
                int goal_dist_sq = gx_off * gx_off + S(ball, 6) * S(ball, 6);
                int in_range = (B(actor, 0x399) != 0) ? (goal_dist_sq <= 0x63FFFFF) : (goal_dist_sq <= 0x63FFFFF);

                if (in_range) {
                    int adiff = (ang_goal - ang_tgt) & 0xFF;
                    int aligned;
                    if ((unsigned int)adiff < 0x81U) {
                        aligned = (adiff < 0x11);
                    } else {
                        aligned = (((ang_tgt - ang_goal) & 0xFF) < 0x11);
                    }
                    if (aligned) {
                        signed short new_z = (S(actor, 6) < 0) ? (S(actor, 6) - 0x100) : (S(actor, 6) + 0x100);
                        tmp = new_z;
                        if (((tmp < 0) ? -tmp : tmp) >= 0x8B7) {
                            new_z = (tmp >= 0) ? 0x8B6 : -0x8B6;
                        }
                        S(actor, 0x3BE) = new_z;
                    }
                }
            }
        }
    }

    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    dx = S(ball, 2) - S(actor, 0x3BC);
    dz = S(ball, 6) - S(actor, 0x3BE);
    dist_sq = dx * dx + dz * dz;
    if (dist_sq <= 0x53DCA1) {
        ang = func_8005B36C(S(ball, 2), S(ball, 6), S(actor, 2), S(actor, 6)) & 0xFF;
        S(actor, 0x3BC) = S(ball, 2) + func_8001E694(ang, 0x9E8);
        S(actor, 0x3BE) = S(ball, 6) + func_8001E5A4(ang, 0x9E8);
    }

    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    dx = S(ball, 2) - S(actor, 2);
    dz = S(ball, 6) - S(actor, 6);
    dist_sq = dx * dx + dz * dz;
    if (dist_sq <= 0x53DCA1) {
        ang = func_8005B36C(S(ball, 2), S(ball, 6), S(actor, 2), S(actor, 6)) & 0xFF;
        S(actor, 0x3BC) = S(ball, 2) + func_8001E694(ang, 0x9E8);
        S(actor, 0x3BE) = S(ball, 6) + func_8001E5A4(ang, 0x9E8);
    }

    /* Clamp to pitch bounds */
    tmp = S(actor, 0x3BC);
    if (((tmp < 0) ? -tmp : tmp) >= 0x32F1) {
        S(actor, 0x3BC) = (tmp <= 0) ? -0x32F0 : 0x32F0;
    }
    tmp = S(actor, 0x3BE);
    if (((tmp < 0) ? -tmp : tmp) >= 0x2101) {
        S(actor, 0x3BE) = (tmp <= 0) ? -0x20D0 : 0x20D0;
    }
}
