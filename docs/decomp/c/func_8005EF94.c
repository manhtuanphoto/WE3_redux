/*
 * func_8005EF94
 *
 * Purpose:
 *   CPU ball-carrier AI decision and dribble/pass/shoot/clear state machine.
 *   Evaluates opponent GK position, nearest marker distance, field coordinates,
 *   player attributes, difficulty level (D_800E6CB2), and AI timers (D_8010C1D8)
 *   to decide whether to shoot, pass, clear, cross, sprint-dribble, or steer
 *   around nearby opponents via 8-sector avoidance weights (D_8010C1D8+0x16..0x1D).
 *
 * Arguments:
 *   actor - pointer to the ball-carrying player's 1000-byte actor struct.
 *
 * Return value:
 *   void
 *
 * Scratchpad reads/writes:
 *   Reads:
 *     0x01A + side*2 (u16) - button/action mask A (shoot/long-pass?)
 *     0x01E + side*2 (u16) - button/action mask B (through/short-pass?)
 *     0x022 + side*2 (u16) - dash/sprint button mask
 *     0x290          (s32) - frame counter (30 frames/s)
 *     0x2DD + side   (u8)  - team strategy/tempo value (divided by 10)
 *     0x2F4          (ptr) - pointer to ball actor
 *     0x30B          (u8)  - match/set-piece mode flag (compared == 4)
 *     0x31E + (dir^1)*2 + i (u8) - opponent GK / key defender actor indices
 *     0x3D0          (u8)  - flag suppressing close-range shot checks when != 0
 *
 * Actor fields reads/writes:
 *   Reads:
 *     +0x002 (s16) x position (250 raw = 1 m)
 *     +0x006 (s16) z position (250 raw = 1 m)
 *     +0x008 (s32) x velocity
 *     +0x010 (s32) z velocity
 *     +0x38D (u8)  actor id
 *     +0x38E (u16) current anim id
 *     +0x390 (u8)  anim frame
 *     +0x391 (u8)  role/position id
 *     +0x392 (u8)  team slot (0..10)
 *     +0x396 (u8)  state
 *     +0x398 (u8)  side (0/1)
 *     +0x399 (u8)  attack direction flag (0 = +X, 1 = -X)
 *     +0x3A0 (u32) speed
 *     +0x3AA (u8)  body heading (0..255 = 360 deg)
 *   Writes:
 *     +0x3C2 (u16) cleared to 0 on special action exit
 *     +0x3D4 (u16) OR'd with Scratchpad[0x22 + side*2] during sprint dribble
 *     +0x3D6 (u16) set to Scratchpad[0x1A + side*2] on special action exit
 *   Ball actor reads:
 *     +0x002 (s16) ball x position
 *     +0x3A8 (u8)  ball heading
 *
 * Globals read/written:
 *   D_800FF748 - 24-element actor array (stride 1000 = 0x3E8 bytes)
 *   D_8010C1D8 - AI ball-carrier state/timer struct:
 *                  +0x00 u8, +0x02 u8 (pass target), +0x03 u8,
 *                  +0x0D+side u8, +0x11+side u8, +0x13 u8,
 *                  +0x14 u8 (AI action: 0..5), +0x15 u8 (AI substate),
 *                  +0x16..0x1D u8[8] (8-sector avoidance flags),
 *                  +0x20 s32, +0x24 s32 (wide/flank timer), +0x28 s32 (hold timer)
 *   D_8010C328 - player stat/AI attribute table (stride 12 bytes/slot, 264 bytes/side)
 *   D_800E6CB0 - u8 turn-dribble step counter
 *   D_800E6CB1 - u8 burst-dribble frame counter (0..8)
 *   D_800E6CB2 - u8 AI difficulty level (0, 1, 2)
 *   D_800C84A8..D_800C84E8 - direction-evaluation tables passed to func_800653D4
 *   D_800C84F0 - u8 table indexed by Scratchpad[0x2DD + side] / 10
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   Positions (x/z):
 *     0x0400 (1024)  =  4.10 m    0x06B7 (1719)  =  6.88 m
 *     0x08B6 (2230)  =  8.92 m    0x09B6 (2486)  =  9.94 m
 *     0x0BC0 (3008)  = 12.03 m    0x0CB6 (3254)  = 13.02 m
 *     0x0CF4 (3316)  = 13.26 m    0x0DF4 (3572)  = 14.29 m
 *     0x0FF4 (4084)  = 16.34 m    0x1080 (4224)  = 16.90 m
 *     0x110B (4363)  = 17.45 m    0x12F4 (4852)  = 19.41 m
 *     0x1601 (5633)  = 22.53 m    0x16F4 (5876)  = 23.50 m
 *     0x1780 (6016)  = 24.06 m    0x18E3 (6371)  = 25.48 m
 *     0x18F5 (6389)  = 25.56 m    0x1991 (6545)  = 26.18 m
 *     0x1C80 (7296)  = 29.18 m    0x1CE2 (7394)  = 29.58 m
 *     0x1D00 (7424)  = 29.70 m    0x1DE2 (7650)  = 30.60 m
 *     0x1E01 (7681)  = 30.72 m    0x1FE3 (8163)  = 32.65 m
 *     0x20E3 (8419)  = 33.68 m    0x21E2 (8674)  = 34.70 m
 *     0x22E3 (8931)  = 35.72 m    0x23E2 (9186)  = 36.74 m
 *     0x2909 (10505) = 42.02 m    0x2C09 (11273) = 45.09 m
 *     0x2E08 (11784) = 47.14 m    0x2EA0 (11936) = 47.74 m
 *     0x2F08 (12040) = 48.16 m    0x2F20 (12064) = 48.26 m
 *     0x3021 (12321) = 49.28 m    0x3320 (13088) = 52.35 m (goal line)
 *   Durations (frames at 30 Hz):
 *     30 (0x1E) = 1.00 s    45 (0x2D) = 1.50 s    50 (0x32) = 1.67 s
 *     60 (0x3C) = 2.00 s   120 (0x78) = 4.00 s   181 (0xB5) = 6.03 s
 *     240 (0xF0) = 8.00 s  361 (0x169) = 12.03 s
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))

#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned char D_8010C1D8[];
extern unsigned char D_8010C328[];
extern unsigned char D_800E6CB0;
extern unsigned char D_800E6CB1;
extern unsigned char D_800E6CB2;
extern unsigned char D_800C84A8[];
extern unsigned char D_800C84B0[];
extern unsigned char D_800C84B8[];
extern unsigned char D_800C84C0[];
extern unsigned char D_800C84C8[];
extern unsigned char D_800C84D0[];
extern unsigned char D_800C84D8[];
extern unsigned char D_800C84DC[];
extern unsigned char D_800C84E0[];
extern unsigned char D_800C84E8[];
extern unsigned char D_800C84F0[];

extern int func_8001E6B8(int n);
extern unsigned int func_80049398(unsigned char *actor, unsigned int heading, unsigned int dist_sq);
extern int func_80054F1C(unsigned char *actor);
extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);
extern unsigned int func_8005B7CC(unsigned int target, unsigned int cur, unsigned int step);
extern unsigned int func_8005CE14(unsigned int actor_id);
extern unsigned int func_8005CE30(unsigned int actor_id);
extern void func_8005E09C(unsigned char *actor, unsigned int heading, unsigned int dash_flag);
extern void func_8005E634(unsigned int arg);
extern void func_80064B44(unsigned char *actor);
extern int func_800652EC(unsigned char *actor, unsigned int thresh);
extern unsigned int func_800653D4(unsigned char *table, unsigned int arg1, unsigned int arg2);
extern int func_80065440(unsigned char *actor, unsigned int heading, unsigned int angle_tol, unsigned int dist_sq);
extern unsigned int func_800655BC(unsigned char *actor, unsigned int heading, unsigned int arg2, unsigned int arg3);
extern int func_800658D0(unsigned char *actor, unsigned int arg1);
extern int func_80065CB8(unsigned char *actor, unsigned char *other);
extern int func_80066150(unsigned char *actor, unsigned char *other);
extern int func_80066414(unsigned char *actor, unsigned char *other);
extern void func_800666DC(unsigned char *actor, unsigned int other_id, unsigned int arg2, unsigned int arg3);
extern void func_800669DC(unsigned char *actor, unsigned int arg1);
extern int func_80067E7C(unsigned char *actor);

static int abs_s16(int v)
{
    return (v < 0) ? -v : v;
}

static unsigned int angle_diff(unsigned int a, unsigned int b)
{
    unsigned int d = (a - b) & 0xFF;
    if (d < 0x81) {
        return d;
    }
    return (b - a) & 0xFF;
}

static int ball_in_own_half(unsigned char *actor)
{
    unsigned char *ball = (unsigned char *)W(Scratchpad, 0x2F4);
    unsigned int dir = B(actor, 0x399);

    if (dir == 0 && S(ball, 2) < 0) {
        return 1;
    }
    if (dir == 1 && S(ball, 2) > 0) {
        return 1;
    }
    return 0;
}

void func_8005EF94(unsigned char *actor)
{
    unsigned char *other;
    unsigned int stat_off;
    int denom;
    int base_dist;
    unsigned int dist_sq;
    unsigned int heading;
    int tmp_s0;
    int tmp_s2;
    unsigned char dash_flag;
    int i;

    /* Check opponent goalkeeper state and whether carrier is facing away from GK */
    other = D_800FF748 + (B(Scratchpad, 0x31E + ((B(actor, 0x399) ^ 1) << 1)) % 24) * 1000;
    {
        unsigned int gk_state = B(other, 0x396);
        if (gk_state >= 12 && (gk_state < 14 || gk_state == 15)) {
            unsigned int gk_ang = func_8005B36C(S(actor, 2), S(actor, 6), S(other, 2), S(other, 6));
            if (angle_diff(gk_ang, B(actor, 0x3AA)) >= 0x41) {
                if (W(D_8010C1D8, 0x28) != 0) {
                    W(D_8010C1D8, 0x28) -= 1;
                }
            }
        }
    }

    if (B(D_8010C1D8, 0x0D + B(actor, 0x398)) == 6) {
        if (func_8001E6B8(3) == 0) {
            W(D_8010C1D8, 0x28) += 1;
        }
    }

    /* Force action 2 if hold timer >= 361 frames (12.03 s) */
    if (SW(D_8010C1D8, 0x28) >= 0x169) {
        if (func_8001E6B8(10) == 0) {
            B(D_8010C1D8, 0x14) = 2;
            B(D_8010C1D8, 0x15) = 0;
            return;
        }
    }

    /* Burst-dribble trigger check */
    stat_off = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
    switch (W(D_8010C328, stat_off) & 0xF) {
    case 6:
        denom = 20;
        break;
    case 7:
        denom = 10;
        break;
    case 8:
        denom = 5;
        break;
    case 9:
        denom = 2;
        break;
    default:
        denom = 100;
        break;
    }

    base_dist = (D_800E6CB2 == 2) ? 0xC4000 : 0;
    if (ball_in_own_half(actor)) {
        base_dist += 0x100000;
    }

    if (func_8001E6B8(denom) == 0) {
        unsigned char *ball = (unsigned char *)W(Scratchpad, 0x2F4);
        unsigned int dir = B(actor, 0x399);
        int ball_ok = (dir == 0) ? (S(ball, 2) >= -0xBC0) : (S(ball, 2) < 0xBC1);

        if (abs_s16(S(actor, 2)) < 0x23E2 && ball_ok) {
            if (angle_diff(dir << 7, B(actor, 0x3AA)) < 0x10) {
                if (func_80065440(actor, dir << 7, 0x28, (unsigned int)(base_dist + (denom << 10) + 0x790000)) != 0
                    && func_800652EC(actor, 0x200) == 0) {
                    D_800E6CB1 = 1;
                    func_8005E09C(actor, (dir << 7) & 0x80, 1);
                }
            }
        }
    }

    /* Active burst-dribble handling */
    if (D_800E6CB1 != 0) {
        unsigned int dir = B(actor, 0x399);
        unsigned int tol;

        func_8005E09C(actor, (dir << 7) & 0x80, 0);
        tol = (abs_s16(S(actor, 6)) < 0x1900) ? 0x30 : 0x18;
        if (angle_diff(B(actor, 0x3AA), dir << 7) < tol) {
            H(actor, 0x3D4) |= H(Scratchpad, 0x22 + B(actor, 0x398) * 2);
        }

        D_800E6CB1++;
        if (D_800E6CB1 >= 9) {
            D_800E6CB1 = 0;
        }
        if (abs_s16(S(actor, 2)) >= 0x23E3) {
            D_800E6CB1 = 0;
        }
        if (B(D_8010C1D8, 0) != 0 && abs_s16(S(actor, 2)) >= 0x20E3) {
            D_800E6CB1 = 0;
        }
        if (D_800E6CB2 == 2 && D_800E6CB1 != 0) {
            if (angle_diff(dir << 7, B(actor, 0x3AA)) >= 0x15) {
                D_800E6CB1++;
            }
        }
        if (B(D_8010C1D8, 0x13) != 0) {
            D_800E6CB1 = 0;
        }
        return;
    }

    if (B(D_8010C1D8, 0x13) != 0) {
        W(D_8010C1D8, 0x28) += 1;
    }

    dash_flag = 0;
    W(D_8010C1D8, 0x28) += 1;

    if (W(D_8010C1D8, 0x24) != 0) {
        W(D_8010C1D8, 0x24) += 1;
        if (W(D_8010C1D8, 0x24) >= 0xB5) { /* 181 frames = 6.03 s */
            W(D_8010C1D8, 0x24) = 0;
        }
        if (abs_s16(S(actor, 6)) < 0x400) { /* 4.10 m */
            W(D_8010C1D8, 0x24) = 0;
        }
    }
    if (abs_s16(S(actor, 6)) >= 0x1D01) { /* 29.70 m */
        W(D_8010C1D8, 0x24) = 1;
    }

    /* Close-to-goal shooting triggers */
    {
        unsigned int nibble = W(D_8010C328, stat_off) & 0xF;
        if (func_8001E6B8(200) < (int)nibble && abs_s16(S(actor, 6)) < 0x9B6) {
            int fwd_x = (B(actor, 0x399) == 0) ? S(actor, 2) : -S(actor, 2);
            if (fwd_x >= 0x22E3) { /* >= 35.72 m */
                B(D_8010C1D8, 0x14) = 4;
                B(D_8010C1D8, 0x15) = 0;
                return;
            }
        }
    }

    {
        unsigned char *ball = (unsigned char *)W(Scratchpad, 0x2F4);
        unsigned int dir = B(actor, 0x399);
        int deep_ball = (dir == 0) ? (S(ball, 2) >= 0x21E3) : (S(ball, 2) < -0x21E2);

        if (deep_ball && B(Scratchpad, 0x30B) != 4) {
            unsigned int nibble = W(D_8010C328, stat_off) & 0xF;
            if (func_8001E6B8(100) < (int)nibble) {
                if (angle_diff(dir << 7, B(ball, 0x3A8)) < 0x30) {
                    B(D_8010C1D8, 0x14) = 4;
                    B(D_8010C1D8, 0x15) = 0;
                    return;
                }
            }
        }
    }

    /* Distance squared to nearest marker/opponent */
    other = D_800FF748 + (func_8005CE30(B(actor, 0x38D)) % 24) * 1000;
    {
        int dx = S(actor, 2) - S(other, 2);
        int dz = S(actor, 6) - S(other, 6);
        dist_sq = (unsigned int)(dx * dx + dz * dz);
    }

    if (func_8001E6B8(15 - D_800E6CB2 * 3) == 0) {
        if (dist_sq <= 0x8FFFFF && abs_s16(S(actor, 6)) < 0xDF4) {
            int fwd_x = (B(actor, 0x399) == 0) ? S(actor, 2) : -S(actor, 2);
            if (fwd_x >= 0x21E3) {
                B(D_8010C1D8, 0x14) = 4;
                B(D_8010C1D8, 0x15) = 1;
                return;
            }
        }
    }

    if (dist_sq <= 0x23FFFF) {
        if (abs_s16(S(actor, 6)) >= 0xDF4) {
            B(D_8010C1D8, 0x14) = 4;
            B(D_8010C1D8, 0x15) = 1;
            return;
        }
        {
            int fwd_x = (B(actor, 0x399) == 0) ? S(actor, 2) : -S(actor, 2);
            if (fwd_x < 0x21E3) {
                B(D_8010C1D8, 0x14) = 4;
                B(D_8010C1D8, 0x15) = 1;
                return;
            }
        }
    }

    {
        int fwd_x = (B(actor, 0x399) == 0) ? S(actor, 2) : -S(actor, 2);
        if (abs_s16(S(actor, 6)) < 0xCF4 && fwd_x >= 0x1CE3 && B(Scratchpad, 0x3D0) == 0) {
            unsigned int dir = B(actor, 0x399);
            if (angle_diff(dir << 7, B(actor, 0x3AA)) < 0x20) {
                if (func_80065440(actor, dir << 7, 8, 0x240000) != 0) {
                    if (abs_s16(S(actor, 6)) < 0xDF4 && fwd_x >= 0x21E3) {
                        B(D_8010C1D8, 0x14) = 4;
                        B(D_8010C1D8, 0x15) = 1;
                        return;
                    }
                }
            }
            {
                unsigned int nibble = W(D_8010C328, stat_off) & 0xF;
                if (func_8001E6B8(200) < (int)(nibble << 1)
                    && abs_s16(S(actor, 2)) >= 0x22E3
                    && abs_s16(S(actor, 6)) < 0xFF4) {
                    B(D_8010C1D8, 0x14) = 4;
                    B(D_8010C1D8, 0x15) = 1;
                    return;
                }
            }
            {
                unsigned int nibble = W(D_8010C328, stat_off) & 0xF;
                if (func_8001E6B8(100 - nibble * 5) == 0 && B(actor, 0x391) >= 6) {
                    B(D_8010C1D8, 0x14) = 4;
                    B(D_8010C1D8, 0x15) = 1;
                    return;
                }
            }
            if (B(D_8010C1D8, 0x11 + B(actor, 0x398)) != 0) {
                unsigned int nibble = W(D_8010C328, stat_off) & 0xF;
                if (func_8001E6B8(100 - nibble * 5) == 0) {
                    if (abs_s16(S(actor, 6)) < 0xCF4 && fwd_x >= 0x1CE3) {
                        B(D_8010C1D8, 0x14) = 4;
                        B(D_8010C1D8, 0x15) = 1;
                        return;
                    }
                }
            }
        }
    }

    {
        int fwd_x = (B(actor, 0x399) == 0) ? S(actor, 2) : -S(actor, 2);
        if (abs_s16(S(actor, 6)) < 0x8B6 && fwd_x >= 0x18E3) {
            unsigned int nb0 = W(D_8010C328, stat_off) & 0xF;
            unsigned int nb4;
            if (func_8001E6B8(100 - nb0 * 5) == 0) {
                B(D_8010C1D8, 0x14) = 4;
                B(D_8010C1D8, 0x15) = 0;
                return;
            }
            nb4 = W(D_8010C328, stat_off + 4) & 0xF; /* D_8010C32C */
            if (func_8001E6B8(100 - nb4 * 5) == 0) {
                B(D_8010C1D8, 0x14) = 4;
                B(D_8010C1D8, 0x15) = 0;
                return;
            }
        }
    }

    /* Own deep-half zone check */
    {
        int fwd_x = (B(actor, 0x399) == 0) ? S(actor, 2) : -S(actor, 2);
        if (fwd_x < -0x1CE2) { /* < -29.58 m */
            B(D_8010C1D8, 0x14) = 3;
            B(D_8010C1D8, 0x15) = 0;
            return;
        }
    }

    {
        unsigned int tbl_val = D_800C84F0[B(Scratchpad, 0x2DD + B(actor, 0x398)) / 10];
        if (tbl_val == 0) {
            denom = 9;
        } else if (tbl_val == 10) {
            denom = 5;
        } else if (tbl_val == 20) {
            denom = 7;
        } else {
            denom = 5;
        }
        if (denom < B(D_8010C1D8, 3) && ball_in_own_half(actor)) {
            B(D_8010C1D8, 0x14) = 5;
            B(D_8010C1D8, 0x15) = 0;
            return;
        }
    }

    /* Long-range burst / angled shot / corner shot checks */
    denom = 20 - (W(D_8010C328, stat_off) & 0xF) * 2;
    if (func_8001E6B8(denom) == 0 && B(D_8010C1D8, 0) != 0) {
        unsigned int dir = B(actor, 0x399);
        if (func_80065440(actor, dir << 7, 0x20, 0x640000) != 0
            && abs_s16(S(actor, 2)) < 0x1DE2
            && abs_s16(S(actor, 6)) < 0xDF4) {
            func_8005E09C(actor, (dir << 7) & 0x80, 1);
            D_800E6CB1 = 5;
            return;
        } else {
            int goal_x = (dir == 0) ? 0x3320 : -0x3320; /* 52.35 m */
            heading = func_8005B36C(S(actor, 2), S(actor, 6), goal_x, 0);
            if (func_80065440(actor, heading & 0xFF, 0x28, (D_800E6CB2 << 16) + 0x790000) != 0
                && abs_s16(S(actor, 2)) < 0x21E2) {
                dash_flag = (angle_diff(B(actor, 0x3AA), heading) < 0x20) ? 1 : 0;
                func_8005E09C(actor, heading & 0xFF, dash_flag);
                D_800E6CB1 = 5;
                return;
            } else {
                int fwd_x = (dir == 0) ? S(actor, 2) : -S(actor, 2);
                if (abs_s16(S(actor, 6)) < 0xFF4 && fwd_x >= 0x23E3) {
                    unsigned int nb3 = B(D_8010C328, stat_off + 3) & 0xF; /* D_8010C32B */
                    unsigned int nb0;
                    if (func_8001E6B8(100 - nb3 * 10) == 0) {
                        B(D_8010C1D8, 0x14) = 4;
                        B(D_8010C1D8, 0x15) = 0;
                        return;
                    }
                    nb0 = W(D_8010C328, stat_off) & 0xF;
                    if (func_8001E6B8(100 - nb0 * 5) == 0) {
                        B(D_8010C1D8, 0x14) = 4;
                        B(D_8010C1D8, 0x15) = 0;
                        return;
                    }
                }
            }
        }
    }

    /* Wing-zone follow-ball heading check */
    if (abs_s16(S(actor, 6)) >= 0x8B7 && abs_s16(S(actor, 2)) < 0x2E08) {
        unsigned char *ball = (unsigned char *)W(Scratchpad, 0x2F4);
        unsigned int dir = B(actor, 0x399);
        int fwd_ball = (dir == 0) ? (S(ball, 2) >= 0x1781) : (S(ball, 2) < -0x1780);

        if (fwd_ball) {
            unsigned int ball_dir = B(ball, 0x3A8);
            if (func_80065440(actor, ball_dir, D_800E6CB2 * 4 + 0x40, 0xA90000) != 0) {
                unsigned int d = (abs_s16(S(actor, 2)) < 0x2C09) ? 1 : 0;
                func_8005E09C(actor, B(ball, 0x3A8), d);
                return;
            }
            if (func_80065440(actor, ball_dir, 0x30, 0x640000) != 0) {
                func_8005E09C(actor, B(ball, 0x3A8), 0);
                return;
            }
        }
    }

    func_80064B44(actor);

    if (W(D_8010C1D8, 0x24) != 0) {
        unsigned char *tbl;
        if (S(actor, 6) > 0) {
            tbl = (B(actor, 0x399) == 0) ? D_800C84B8 : D_800C84C8;
        } else {
            tbl = (B(actor, 0x399) == 0) ? D_800C84C0 : D_800C84D0;
        }
        func_800653D4(tbl, 4, 2);
    }

    /* Midfield/forward pass scan */
    if (B(D_8010C1D8, 0) == 0) {
        unsigned char *ball = (unsigned char *)W(Scratchpad, 0x2F4);
        unsigned int dir = B(actor, 0x399);
        unsigned int role = B(actor, 0x391);
        int role_mid = (role >= 17 && role < 26) ? 1 : 0;
        int ball_back = (dir == 0) ? (S(ball, 2) < -0xBC0) : (S(ball, 2) >= 0xBC1);

        if (ball_back && func_800652EC(actor, ((role_mid * 9) << 18) | 0x600) != 0) {
            tmp_s0 = func_800655BC(actor, dir << 7, 3, 0x38) & 0xFF;
            if (tmp_s0 != 0xFF) {
                B(D_8010C1D8, 0x14) = 2;
                B(D_8010C1D8, 0x15) = 1;
                B(D_8010C1D8, 2) = (unsigned char)tmp_s0;
                return;
            }
            tmp_s2 = func_800655BC(actor, dir << 7, 1, 0x20) & 0xFF;
            if (tmp_s2 != tmp_s0) {
                B(D_8010C1D8, 0x14) = 2;
                B(D_8010C1D8, 0x15) = 1;
                B(D_8010C1D8, 2) = (unsigned char)tmp_s2;
                return;
            }
        }
        if (ball_in_own_half(actor) && role_mid != 0) {
            tmp_s0 = func_800655BC(actor, dir << 7, 1, 0x20) & 0xFF;
            if (tmp_s0 != 0xFF) {
                B(D_8010C1D8, 0x14) = 2;
                B(D_8010C1D8, 0x15) = 1;
                B(D_8010C1D8, 2) = (unsigned char)tmp_s0;
                return;
            }
            tmp_s2 = func_800655BC(actor, dir << 7, 1, 0x30) & 0xFF;
            if (tmp_s2 != tmp_s0) {
                B(D_8010C1D8, 0x14) = 2;
                B(D_8010C1D8, 0x15) = 1;
                B(D_8010C1D8, 2) = (unsigned char)tmp_s2;
                return;
            }
        }
    }

    /* Teammate pass / through-ball evaluation in attacking zone */
    {
        unsigned char *ball = (unsigned char *)W(Scratchpad, 0x2F4);
        unsigned int dir = B(actor, 0x399);
        int trigger = 0;

        if ((dir == 0 && S(ball, 2) >= 0x1781) || (dir == 1 && S(ball, 2) < -0x1780)) {
            trigger = 1;
        } else if (func_8001E6B8(10) == 0) {
            if ((dir == 0 && S(ball, 2) >= 0xBC1) || (dir == 1 && S(ball, 2) < -0xBC0)) {
                trigger = 1;
            }
        }

        if (trigger && B(actor, 0x396) != 3) {
            unsigned int key = 4 - (H(D_8010C328, stat_off + 6) & 0xF); /* D_8010C32E */
            unsigned int base_idx;

            switch (key) {
            case 0:
                denom = 2;
                break;
            case 1:
                denom = 3;
                break;
            case 2:
                denom = 5;
                break;
            case 3:
                denom = 8;
                break;
            case 4:
                denom = 12;
                break;
            default:
                denom = 16;
                break;
            }

            if (D_800E6CB2 == 0) {
                denom <<= 1;
            } else if (D_800E6CB2 == 2) {
                denom /= 3;
            }

            if (func_8001E6B8(denom) == 0) {
                base_idx = func_8005CE14(B(actor, 0x38D)) & 0xFF;
                other = D_800FF748 + base_idx * 1000 + 10000;
                for (i = 0; i < 10; i++) {
                    if (func_80065CB8(actor, other) != 0) {
                        func_800666DC(actor, B(other, 0x38D), 1, H(Scratchpad, 0x1E + B(actor, 0x398) * 2));
                        return;
                    }
                    other -= 1000;
                }
            }

            if (func_8001E6B8(denom) == 0 && angle_diff(dir << 7, B(actor, 0x3AA)) < 0x20) {
                base_idx = func_8005CE14(B(actor, 0x38D)) & 0xFF;
                other = D_800FF748 + base_idx * 1000 + 1000;
                for (i = 0; i < 10; i++) {
                    if (func_80066414(actor, other) != 0) {
                        func_800666DC(actor, B(other, 0x38D), 0xFF, H(Scratchpad, 0x1A + B(actor, 0x398) * 2));
                        return;
                    }
                    other += 1000;
                }
            }

            base_idx = func_8005CE14(B(actor, 0x38D)) & 0xFF;
            other = D_800FF748 + base_idx * 1000 + 1000;
            for (i = 0; i < 10; i++) {
                if (func_80066150(actor, other) != 0) {
                    func_800666DC(actor, B(other, 0x38D), 1, H(Scratchpad, 0x1E + B(actor, 0x398) * 2));
                    return;
                }
                other += 1000;
            }

            if (func_8001E6B8(denom) == 0) {
                if (S(actor, 6) >= 0x1081 && angle_diff(0xC0, B(actor, 0x3AA)) < 0x20) {
                    B(D_8010C1D8, 0x14) = 2;
                    B(D_8010C1D8, 0x15) = 0x20;
                    return;
                }
                if (S(actor, 6) < -0x1080 && angle_diff(0x40, B(actor, 0x3AA)) < 0x20) {
                    B(D_8010C1D8, 0x14) = 2;
                    B(D_8010C1D8, 0x15) = 0x20;
                    return;
                }
            }
        }
    }

    /* Endline cutback / cross-or-shoot zone */
    if (abs_s16(S(actor, 2)) >= 0x2E09 && abs_s16(S(actor, 6)) < 0x12F4) {
        unsigned char *ball = (unsigned char *)W(Scratchpad, 0x2F4);
        unsigned int h;

        tmp_s0 = func_800655BC(actor, B(ball, 0x3A8), 0, 0x20) & 0xFF;
        if (tmp_s0 != 0xFF) {
            B(D_8010C1D8, 0x14) = 2;
            B(D_8010C1D8, 0x15) = 1;
            B(D_8010C1D8, 2) = (unsigned char)tmp_s0;
            return;
        }
        if (abs_s16(S(actor, 2)) >= 0x2E09) {
            h = (S(actor, 6) >= 0) ? 0xC0 : 0x40;
        } else {
            h = B(ball, 0x3A8);
        }
        func_8005E09C(actor, h, 0);
        if (func_800652EC(actor, D_800E6CB2 * 0xC0 + 0x200) != 0) {
            B(D_8010C1D8, 0x14) = 4;
            B(D_8010C1D8, 0x15) = 0;
        }
        return;
    }

    /* Main substate dispatch (D_8010C1D8+0x15) */
    switch (B(D_8010C1D8, 0x15)) {
    case 0:
    {
        unsigned int dir = B(actor, 0x399);
        unsigned char *ball = (unsigned char *)W(Scratchpad, 0x2F4);
        int ax;

        if (H(actor, 0x38E) == 0xC && B(actor, 0x390) >= 10) {
            tmp_s0 = func_800655BC(actor, B(actor, 0x3AA), 2, 0x18) & 0xFF;
            if (tmp_s0 != 0xFF) {
                B(D_8010C1D8, 0x14) = 2;
                B(D_8010C1D8, 0x15) = 1;
                B(D_8010C1D8, 2) = (unsigned char)tmp_s0;
                return;
            }
        }

        heading = func_800653D4((dir == 0) ? D_800C84A8 : D_800C84B0, 2, 0);
        if ((heading & 0xFF) == 0xFF) {
            heading = func_800653D4((dir == 0) ? D_800C84A8 : D_800C84B0, 8, 1);
        } else if (B(actor, 0x396) != 0 && B(actor, 0x396) != 2) {
            if ((dir == 0 && S(ball, 2) > 0) || (dir == 1 && S(ball, 2) < 0)) {
                dash_flag = 1;
            }
        }

        if ((heading & 0xFF) == (dir << 7)) {
            if (W(D_8010C1D8, 0x28) != 0) {
                W(D_8010C1D8, 0x28) -= 1;
            }
        }

        denom = 0;
        if (ball_in_own_half(actor)) {
            ax = abs_s16(S(actor, 2));
            if (ax >= 0x1991) {
                denom = -30;
            } else if (ax >= 0x110B) {
                denom = -20;
            }
            if (B(actor, 0x391) >= 17 && B(actor, 0x391) < 26) {
                if (func_800652EC(actor, 0x500) != 0) {
                    denom = -40;
                }
            }
            if (angle_diff(dir << 7, B(actor, 0x3AA)) >= 0x51) {
                if (func_800652EC(actor, 0x300) != 0) {
                    B(D_8010C1D8, 0x14) = 2;
                    B(D_8010C1D8, 0x15) = 0;
                    return;
                }
            }
        } else {
            unsigned int nibble = W(D_8010C328, stat_off) & 0xF;
            ax = abs_s16(S(actor, 2));
            if (ax >= 0x1991) {
                denom = (int)(nibble * 6);
            } else if (ax >= 0x110B) {
                denom = (int)(nibble * 5);
            } else {
                denom = (int)(nibble * 4);
            }
        }

        if (D_800E6CB2 == 2) {
            denom /= 2;
        }

        if (B(D_8010C1D8, 0) == 0 && (denom + 30) < SW(D_8010C1D8, 0x28)) {
            B(D_8010C1D8, 0x14) = 2;
            B(D_8010C1D8, 0x15) = 0;
            SW(D_8010C1D8, 0x28) -= 16;
            if (SW(D_8010C1D8, 0x28) < 0x3D) {
                return;
            }
            tmp_s0 = func_800655BC(actor, dir << 7, 0, 0x20) & 0xFF;
            if (tmp_s0 == 0xFF) {
                return;
            }
            B(D_8010C1D8, 0x14) = 2;
            B(D_8010C1D8, 0x15) = 1;
            B(D_8010C1D8, 2) = (unsigned char)tmp_s0;
            return;
        }

        if (B(Scratchpad, 0x30B) == 4) {
            B(D_8010C1D8, 0x14) = 1;
            B(D_8010C1D8, 0x15) = 4;
            return;
        }

        if ((heading & 0xFF) != 0xFF && func_80054F1C(actor) == 0) {
            break;
        }

        if (func_8001E6B8(2) != 0 && abs_s16(S(actor, 6)) < 0xDF4) {
            int fwd_x = (dir == 0) ? S(actor, 2) : -S(actor, 2);
            if (fwd_x >= 0x21E3) {
                B(D_8010C1D8, 0x14) = 4;
                B(D_8010C1D8, 0x15) = 0;
                return;
            }
            if ((heading & 0xFF) == 0xFF) {
                heading = dir << 7;
            }
        } else if ((heading & 0xFF) == 0xFF) {
            heading = dir << 7;
        }

        tmp_s2 = (int)func_80049398(actor, heading & 0xFF, 0x100000);
        if ((tmp_s2 & 0xFF) == (heading & 0xFF)) {
            break;
        }
        if (abs_s16(S(actor, 6)) >= 0x1D01 && W(actor, 0x3A0) != 0) {
            return;
        }
        heading = (unsigned int)tmp_s2;
        if (angle_diff(B(actor, 0x3AA), heading) < 0x10) {
            return;
        }
        func_8005E09C(actor, heading & 0xFF, 0);
        return;
    }

    case 2:
    {
        unsigned int dir = B(actor, 0x399);
        unsigned char *ball = (unsigned char *)W(Scratchpad, 0x2F4);

        W(D_8010C1D8, 0x24) = 0;
        if (D_800E6CB2 == 0) {
            base_dist = -0x40000;
        } else if (D_800E6CB2 == 1) {
            base_dist = ((SW(Scratchpad, 0x290) % 240) < 120) ? -0x10000 : 0x40000;
        } else {
            base_dist = 0x40000;
        }

        if (ball_in_own_half(actor)) {
            base_dist += 0x90000;
        }

        for (i = 0; i < 8; i++) {
            B(D_8010C1D8, 0x16 + i) = 0;
        }

        /* Compute 8-sector avoidance flags from 3 key opponents */
        for (i = 0; i < 3; i++) {
            int dx, dz;

            if (i < 2) {
                unsigned int idx = B(Scratchpad, 0x31E + ((dir ^ 1) << 1) + i);
                other = D_800FF748 + (idx % 24) * 1000;
            } else {
                int idx = func_800658D0(actor, 1);
                other = D_800FF748 + (idx % 24) * 1000;
            }

            dx = S(actor, 2) - S(other, 2);
            dz = S(actor, 6) - S(other, 6);
            dist_sq = (unsigned int)(dx * dx + dz * dz);
            heading = func_8005B36C(S(actor, 2), S(actor, 6), S(other, 2), S(other, 6));

            tmp_s2 = (dist_sq < (unsigned int)(base_dist + 0x100000)) ? 1 : 0;
            if (dist_sq < (unsigned int)((base_dist >> 1) + 0x90000)) {
                tmp_s2 = 8;
            }
            if (dist_sq < (unsigned int)(base_dist / 3 + 0x40000)) {
                tmp_s2 = 0x10;
            }
            if (dist_sq < (unsigned int)(base_dist / 3 + 0x10000)) {
                tmp_s2 = 0x28;
            }

            if ((tmp_s2 & 0xFF) != 0) {
                unsigned int tol = (tmp_s2 & 0xFF) + 0x20;
                int sec;
                for (sec = 0; sec < 8; sec++) {
                    if (angle_diff(heading, (unsigned int)(sec << 5)) < tol) {
                        B(D_8010C1D8, 0x16 + sec) = 1;
                    }
                }
            }
        }

        func_800669DC(actor, 1);

        if (B(actor, 0x396) != 0) {
            unsigned int back = (B(actor, 0x3AA) + 0x80) & 0xFF;
            int sec;
            for (sec = 0; sec < 8; sec++) {
                if (angle_diff(back, (unsigned int)(sec << 5)) < 0x20) {
                    B(D_8010C1D8, 0x16 + sec) = 1;
                }
            }
        }

        heading = func_800653D4((dir == 0) ? D_800C84A8 : D_800C84B0, 8, 0);
        W(D_8010C1D8, 0x24) = 0;

        if (ball_in_own_half(actor)) {
            tmp_s0 = (abs_s16(S(actor, 2)) < 0x110B) ? 0x2D : 0x1E;
            if (func_800652EC(actor, 0x300) != 0) {
                tmp_s0 = 8;
            }
        } else {
            unsigned int nb2 = H(D_8010C328, stat_off + 2) & 0xF; /* D_8010C32A */
            if (nb2 == 6) {
                tmp_s0 = 0x2D;
            } else if (nb2 == 7 || nb2 >= 10) {
                tmp_s0 = 0x32;
            } else {
                tmp_s0 = 0x1E;
            }
        }

        if (D_800E6CB2 == 2) {
            tmp_s0 = (tmp_s0 & 0xFF) >> 1;
        }

        if ((tmp_s0 & 0xFF) < SW(D_8010C1D8, 0x28)) {
            unsigned int tbl_val = D_800C84F0[B(Scratchpad, 0x2DD + B(actor, 0x398)) / 10];
            if (tbl_val == 0) {
                tmp_s0 = 9;
            } else if (tbl_val == 10) {
                tmp_s0 = 5;
            } else if (tbl_val == 20) {
                tmp_s0 = 7;
            }

            if ((unsigned int)(tmp_s0 & 0xFF) < B(D_8010C1D8, 3)) {
                unsigned int ret = func_800655BC(actor, dir << 7, 2, 0x3C) & 0xFF;
                if (ret != 0xFF) {
                    B(D_8010C1D8, 0x14) = 2;
                    B(D_8010C1D8, 0x15) = 1;
                    B(D_8010C1D8, 2) = (unsigned char)ret;
                    return;
                }
            }

            {
                unsigned int arg2;
                unsigned int ret;
                if (ball_in_own_half(actor)) {
                    arg2 = (abs_s16(S(actor, 2)) < 0x110B) ? 2 : 3;
                } else {
                    arg2 = 0;
                }
                ret = func_800655BC(actor, B(actor, 0x3AA), arg2, 0x3C) & 0xFF;
                if (ret != 0xFF) {
                    B(D_8010C1D8, 0x14) = 2;
                    B(D_8010C1D8, 0x15) = 1;
                    B(D_8010C1D8, 2) = (unsigned char)ret;
                    return;
                }
            }

            if (abs_s16(S(actor, 6)) >= 0x1601) {
                if ((dir == 0 && S(ball, 2) > 0) || (dir == 1 && S(ball, 2) < 0)) {
                    B(D_8010C1D8, 0x14) = 2;
                    B(D_8010C1D8, 0x15) = 0x20;
                    return;
                }
            }
        }

        if ((heading & 0xFF) == 0xFF) {
            if (abs_s16(S(actor, 2)) < 0x2F08) {
                unsigned int ang = func_80049398(actor, (dir << 7) & 0x80, 0x90000);
                func_8005E09C(actor, ang & 0xFF, 0);
                if (ball_in_own_half(actor)) {
                    B(D_8010C1D8, 0x14) = 2;
                    B(D_8010C1D8, 0x15) = 0;
                }
                return;
            }
        } else {
            if (angle_diff(dir << 7, heading) >= 0x51
                && func_80065440(actor, heading & 0xFF, 0x20, 0x90000) != 0) {
                heading = 0xFF;
            }
        }
        break;
    }

    case 4:
    {
        unsigned int dir = B(actor, 0x399);
        unsigned char *ball = (unsigned char *)W(Scratchpad, 0x2F4);
        int fwd_x;

        W(D_8010C1D8, 0x20) = 0;
        W(D_8010C1D8, 0x24) = 0;
        heading = 0xFF;

        if (abs_s16(S(actor, 2)) >= 0x23E3 && abs_s16(S(actor, 6)) >= 0xCB7) {
            heading = func_800653D4((S(actor, 6) > 0) ? D_800C84DC : D_800C84D8, 3, 1);
        }

        if ((heading & 0xFF) == 0xFF) {
            unsigned char *tbl;
            if (S(actor, 6) > 0) {
                tbl = (dir == 0) ? D_800C84B8 : D_800C84C8;
            } else {
                tbl = (dir == 0) ? D_800C84C0 : D_800C84D0;
            }
            heading = func_800653D4(tbl, 5, 1);

            if ((heading & 0xFF) == 0xFF) {
                heading = func_800653D4((dir == 0) ? D_800C84A8 : D_800C84B0, 8, 2);
                if ((heading & 0xFF) == 0xFF) {
                    W(D_8010C1D8, 0x20) = 1;
                }
            }
        }

        if (B(Scratchpad, 0x30B) != 4) {
            B(D_8010C1D8, 0x14) = 0;
            B(D_8010C1D8, 0x15) = 0;
        }

        fwd_x = (dir == 0) ? S(actor, 2) : -S(actor, 2);
        if (abs_s16(S(actor, 6)) < 0xCB6 && fwd_x >= 0x1FE3) {
            if (angle_diff(dir << 7, B(actor, 0x3AA)) < 0x10) {
                B(D_8010C1D8, 0x14) = 4;
                B(D_8010C1D8, 0x15) = 1;
                return;
            }
        }

        if (angle_diff(0x40, B(ball, 0x3A8)) < 0x18 || angle_diff(0xC0, B(ball, 0x3A8)) < 0x18) {
            if (SW(D_8010C1D8, 0x28) >= 0xB5
                || func_80065440(actor, (S(actor, 6) >= 0) ? 0xC0 : 0x40, 0x18, 0x100000) != 0) {
                if (func_80067E7C(actor) != 0) {
                    H(actor, 0x3C2) = 0;
                    H(actor, 0x3D6) = H(Scratchpad, 0x1A + B(actor, 0x398) * 2);
                    return;
                }
            }
        }

        {
            int skip_cross = 0;
            if (func_80065440(actor, dir << 7, 0x40, 0x240000) != 0) {
                if ((dir == 0 && S(ball, 2) >= 0xBC1) || (dir == 1 && S(ball, 2) < -0xBC0)) {
                    skip_cross = 1;
                }
            }

            if (!skip_cross && B(actor, 0x396) != 3 && SW(D_8010C1D8, 0x28) >= 0xB5) {
                if (func_8001E6B8(20) == 0) {
                    unsigned int side_h = (S(actor, 6) >= 0) ? 0xC0 : 0x40;
                    tmp_s0 = func_800655BC(actor, side_h, 3, 0x18) & 0xFF;
                    if (tmp_s0 != 0xFF) {
                        B(D_8010C1D8, 0x14) = 2;
                        B(D_8010C1D8, 0x15) = 1;
                        B(D_8010C1D8, 2) = (unsigned char)tmp_s0;
                        return;
                    }
                    {
                        unsigned int ret = func_800655BC(actor, (dir ^ 1) << 7, 1, 0x18) & 0xFF;
                        if ((int)ret != tmp_s0) {
                            B(D_8010C1D8, 0x14) = 2;
                            B(D_8010C1D8, 0x15) = 1;
                            B(D_8010C1D8, 2) = (unsigned char)ret;
                            return;
                        }
                    }
                }
            }
        }

        if ((SW(Scratchpad, 0x290) % 60) == 0 && abs_s16(S(actor, 2)) >= 0x2909) {
            if (func_8001E6B8(2) != 0) {
                func_8005E634((dir == 0) ? 2 : 3);
            }
        }
        break;
    }

    default:
    {
        unsigned int dir = B(actor, 0x399);
        unsigned char *ball = (unsigned char *)W(Scratchpad, 0x2F4);
        int abs_s4_2;
        int neg_s4;
        int deep_ball;

        if (D_800E6CB2 == 0) {
            base_dist = -0x90000;
        } else if (D_800E6CB2 == 1) {
            base_dist = ((SW(Scratchpad, 0x290) % 240) < 120) ? -0x90000 : 0x90000;
        } else {
            base_dist = 0x90000;
        }

        if (ball_in_own_half(actor)) {
            base_dist += 0x90000;
        } else {
            unsigned int nibble = W(D_8010C328, stat_off) & 0xF;
            if (B(D_8010C1D8, 0) != 0) {
                base_dist = 0;
            }
            if (nibble == 7) {
                if ((SW(Scratchpad, 0x290) % 60) < 30) {
                    base_dist -= 0x10000;
                }
            } else if (nibble == 8) {
                if ((SW(Scratchpad, 0x290) % 60) < 30) {
                    base_dist -= 0x24000;
                }
            } else if (nibble == 9) {
                if ((SW(Scratchpad, 0x290) % 60) < 30) {
                    base_dist -= 0x40000;
                }
            }
        }

        for (i = 0; i < 8; i++) {
            B(D_8010C1D8, 0x16 + i) = 0;
        }

        abs_s4_2 = ((base_dist < 0) ? -base_dist : base_dist) << 1;
        neg_s4 = base_dist >> 31;

        /* Predicted-position 8-sector avoidance check against 3 key opponents */
        for (i = 0; i < 3; i++) {
            int dx, dz;
            int px1, pz1, px2, pz2;
            int r;
            unsigned int thresh;

            if (i < 2) {
                unsigned int idx = B(Scratchpad, 0x31E + ((dir ^ 1) << 1) + i);
                other = D_800FF748 + (idx % 24) * 1000;
            } else {
                int idx = func_800658D0(actor, 1);
                other = D_800FF748 + (idx % 24) * 1000;
            }

            /* UNCLEAR: dist_sq adds actor velocity to s6-actor delta rather than subtracting s6 velocity */
            dx = S(other, 2) - S(actor, 2) + SW(actor, 8);
            dz = S(other, 6) - S(actor, 6) + SW(actor, 16);
            dist_sq = (unsigned int)(dx * dx + dz * dz);

            px1 = (signed short)(S(actor, 2) + SW(actor, 8));
            pz1 = (signed short)(S(actor, 6) + SW(actor, 16));
            px2 = (signed short)(S(other, 2) + SW(other, 8));
            pz2 = (signed short)(S(other, 6) + SW(other, 16));
            heading = func_8005B36C(px1, pz1, px2, pz2);

            tmp_s2 = 0;
            r = func_8001E6B8(W(D_8010C328, stat_off) & 0xF);
            thresh = (unsigned int)(abs_s4_2 - ((r << 14) - 0x640000));
            if (dist_sq < thresh) {
                int sec;
                for (sec = 0; sec < 8; sec++) {
                    if (angle_diff(heading, (unsigned int)(sec << 5)) < 0x30) {
                        if (B(D_8010C1D8, 0x16 + sec) == 0) {
                            B(D_8010C1D8, 0x16 + sec) = 1;
                        }
                    }
                }

                if (dist_sq < (unsigned int)(base_dist + 0x190000)) {
                    tmp_s2 = 0;
                }
                if (dist_sq < (unsigned int)(((base_dist - neg_s4) >> 1) + 0xC4000)) {
                    tmp_s2 = 8;
                }
                if (dist_sq < (unsigned int)(base_dist / 3 + 0x40000)) {
                    tmp_s2 = 0x10;
                }
                if (dist_sq <= 0x23FFF) {
                    unsigned int back = (B(actor, 0x3AA) + 0x80) & 0xFF;
                    tmp_s2 = 0x18;
                    for (sec = 0; sec < 8; sec++) {
                        if (angle_diff(back, (unsigned int)(sec << 5)) < 0x20) {
                            B(D_8010C1D8, 0x16 + sec) = 2;
                        }
                    }
                }
                {
                    unsigned int tol = (tmp_s2 & 0xFF) + 0x20;
                    for (sec = 0; sec < 8; sec++) {
                        if (angle_diff(heading, (unsigned int)(sec << 5)) < tol) {
                            B(D_8010C1D8, 0x16 + sec) = 2;
                        }
                    }
                }
            }
        }

        func_800669DC(actor, 2);

        if (abs_s16(S(actor, 6)) < 0x16F4) {
            if (dir == 0) {
                heading = func_800653D4(D_800C84E0, 8, 1);
                if ((heading & 0xFF) == func_800653D4(D_800C84E0, 3, 0) && (heading & 0xFF) == 0) {
                    dash_flag = 1;
                    if (W(D_8010C1D8, 0x28) != 0) {
                        W(D_8010C1D8, 0x28) -= 1;
                    }
                }
            } else {
                heading = func_800653D4(D_800C84E8, 8, 1);
                if ((heading & 0xFF) == func_800653D4(D_800C84E0, 3, 0) && (heading & 0xFF) == 0x80) {
                    dash_flag = 1;
                    if (W(D_8010C1D8, 0x28) != 0) {
                        W(D_8010C1D8, 0x28) -= 1;
                    }
                }
            }
        } else {
            if (dir == 0) {
                heading = func_800653D4(D_800C84A8, 8, 1);
                if ((heading & 0xFF) == func_800653D4(D_800C84A8, 3, 0) && (heading & 0xFF) == 0) {
                    dash_flag = 1;
                    if (W(D_8010C1D8, 0x28) != 0) {
                        W(D_8010C1D8, 0x28) -= 1;
                    }
                }
            } else {
                heading = func_800653D4(D_800C84B0, 8, 1);
                if ((heading & 0xFF) == func_800653D4(D_800C84A8, 3, 0) && (heading & 0xFF) == 0x80) {
                    dash_flag = 1;
                    if (W(D_8010C1D8, 0x28) != 0) {
                        W(D_8010C1D8, 0x28) -= 1;
                    }
                }
            }
        }

        if (ball_in_own_half(actor)) {
            dash_flag = 0;
        }

        if (abs_s16(S(actor, 6)) >= 0x18F5) {
            unsigned int ret = func_800655BC(actor, dir << 7, 2, 0x30) & 0xFF;
            if (ret != 0xFF) {
                B(D_8010C1D8, 0x14) = 2;
                B(D_8010C1D8, 0x15) = 1;
                B(D_8010C1D8, 2) = (unsigned char)ret;
                return;
            }
        }

        if (B(D_8010C1D8, 0) == 0) {
            int check_pass = 0;
            if (func_80065440(actor, dir << 7, 0x40, 0x240000) != 0) {
                check_pass = 1;
            } else if (dir == 0 && S(ball, 2) < 0xBC1) {
                check_pass = 1;
            } else if (dir == 1 && S(ball, 2) >= -0xBC0) {
                check_pass = 1;
            }

            if (check_pass && B(actor, 0x396) != 3) {
                unsigned int nibble = W(D_8010C328, stat_off) & 0xF;
                if (func_800652EC(actor, 0x600) != 0
                    && (int)(nibble * (3 - D_800E6CB2)) < SW(D_8010C1D8, 0x28)) {
                    unsigned int ret = func_800655BC(actor, dir << 7, 2, 0x20) & 0xFF;
                    if (ret != 0xFF) {
                        B(D_8010C1D8, 0x14) = 2;
                        B(D_8010C1D8, 0x15) = 1;
                        B(D_8010C1D8, 2) = (unsigned char)ret;
                        return;
                    }
                }

                if ((int)(nibble * (4 - D_800E6CB2) + 0x1E) < SW(D_8010C1D8, 0x28)) {
                    tmp_s0 = func_800655BC(actor, dir << 7, 1, 0x30) & 0xFF;
                    if (tmp_s0 != 0xFF) {
                        B(D_8010C1D8, 0x14) = 2;
                        B(D_8010C1D8, 0x15) = 1;
                        B(D_8010C1D8, 2) = (unsigned char)tmp_s0;
                        return;
                    }
                    tmp_s2 = func_800655BC(actor, dir << 7, 0, 0x40) & 0xFF;
                    if (tmp_s2 != tmp_s0) {
                        B(D_8010C1D8, 0x14) = 2;
                        B(D_8010C1D8, 0x15) = 1;
                        B(D_8010C1D8, 2) = (unsigned char)tmp_s2;
                        return;
                    }
                    if ((int)(nibble * (9 - D_800E6CB2) + 0x1E) < SW(D_8010C1D8, 0x28)) {
                        unsigned int ret = func_800655BC(actor, dir << 7, 0xFF, 0x60) & 0xFF;
                        if ((int)ret != tmp_s2) {
                            B(D_8010C1D8, 0x14) = 2;
                            B(D_8010C1D8, 0x15) = 1;
                            B(D_8010C1D8, 2) = (unsigned char)ret;
                            return;
                        }
                    }
                }
            }
        }

        deep_ball = (dir == 0) ? (S(ball, 2) >= 0x21E3) : (S(ball, 2) < -0x21E2);

        if ((heading & 0xFF) == 0xFF) {
            unsigned int ret = func_800655BC(actor, dir << 7, 0, 0x30) & 0xFF;
            if (ret != 0xFF) {
                B(D_8010C1D8, 0x14) = 2;
                B(D_8010C1D8, 0x15) = 1;
                B(D_8010C1D8, 2) = (unsigned char)ret;
                return;
            }

            if (abs_s16(S(actor, 2)) < 0x2F20) {
                if (func_80065440(actor, dir << 7, 0x20, 0x190000) != 0) {
                    heading = dir << 7;
                }
            }

            if (abs_s16(S(actor, 6)) < 0x1D00) {
                other = D_800FF748 + (B(Scratchpad, 0x31E + ((dir ^ 1) << 1)) % 24) * 1000;
                heading = func_8005B36C(S(other, 2), S(other, 6), S(actor, 2), S(actor, 6));
                if (angle_diff((dir << 7) + 0x80, heading) < 0x30) {
                    heading = 0xFF;
                }
            }

            if (B(Scratchpad, 0x30B) == 4 || (deep_ball && abs_s16(S(actor, 6)) >= 0x8B7)) {
                if (func_80067E7C(actor) != 0) {
                    H(actor, 0x3C2) = 0;
                    H(actor, 0x3D6) = H(Scratchpad, 0x1A + B(actor, 0x398) * 2);
                    return;
                }
            }
        }

        if (B(actor, 0x396) == 2) {
            W(D_8010C1D8, 0x28) += 3;
        }

        if (B(Scratchpad, 0x30B) == 4 || (deep_ball && abs_s16(S(actor, 6)) >= 0x8B7)) {
            if (abs_s16(S(actor, 6)) >= 0x6B7) {
                if (SW(D_8010C1D8, 0x28) >= 0x79
                    || func_80065440(actor, (S(actor, 6) >= 0) ? 0xC0 : 0x40, 0x18, 0x100000) != 0) {
                    if (func_80067E7C(actor) != 0) {
                        H(actor, 0x3C2) = 0;
                        H(actor, 0x3D6) = H(Scratchpad, 0x1A + B(actor, 0x398) * 2);
                        return;
                    }
                }
            }
        }
        break;
    }
    }

    /* Final dribble steering & movement execution (.L80062774) */
    if (abs_s16(S(actor, 6)) < 0x1C80 && abs_s16(S(actor, 2)) < 0x2EA0) {
        if (D_800E6CB0 != 0) {
            if (D_800E6CB2 == 0) {
                heading = func_8005B7CC(heading & 0xFF, B(actor, 0x3AA), 0x10);
            } else if (D_800E6CB2 == 1) {
                heading = func_8005B7CC(heading & 0xFF, B(actor, 0x3AA), 0x18);
            } else {
                unsigned int step = func_800652EC(actor, 0x300) ? 0x40 : 0x20;
                heading = func_8005B7CC(heading & 0xFF, B(actor, 0x3AA), step);
            }

            func_8005E09C(actor, heading & 0xFF, 0);

            if (H(actor, 0x38E) == 0xC) {
                if (D_800E6CB0 != 0) {
                    D_800E6CB0--;
                }
            } else {
                D_800E6CB0++;
            }

            if (D_800E6CB2 == 0) {
                int limit = 50 - (H(D_8010C328, stat_off + 2) & 0xF) * 2;
                if (limit < (int)D_800E6CB0) {
                    D_800E6CB0 = 0;
                }
            } else if (D_800E6CB2 == 1) {
                int limit = 30 - (H(D_8010C328, stat_off + 2) & 0xF) * 2;
                if (limit < (int)D_800E6CB0) {
                    D_800E6CB0 = 0;
                }
            } else if (D_800E6CB2 == 2) {
                if (D_800E6CB0 >= 4) {
                    D_800E6CB0 = 0;
                }
            }
            return;
        } else {
            switch (H(D_8010C328, stat_off + 2) & 0xF) {
            case 6:
                denom = 5;
                break;
            case 7:
                denom = 10;
                break;
            case 8:
                denom = 15;
                break;
            case 9:
                denom = 20;
                break;
            default:
                denom = 2;
                break;
            }
            if (D_800E6CB2 == 0) {
                denom /= 2;
            } else if (D_800E6CB2 == 1) {
                denom = (denom * 2) / 3;
            }
            if (func_8001E6B8(denom) == 0 && angle_diff(B(actor, 0x3AA), B(actor, 0x399) << 7) < 0x30) {
                D_800E6CB0 = 1;
            }
        }
    }

    if (angle_diff(B(actor, 0x3AA), heading) >= 9) {
        dash_flag = 0;
    }

    {
        unsigned char *ball = (unsigned char *)W(Scratchpad, 0x2F4);
        unsigned int dir = B(actor, 0x399);
        int fwd_ball = (dir == 0) ? (S(ball, 2) >= 0x1781) : (S(ball, 2) < -0x1780);

        if (!fwd_ball && dash_flag == 0) {
            if (func_80065440(actor, dir << 7, 0x40, 0x190000) != 0) {
                heading = dir << 7;
            }
        }
    }

    if ((heading & 0xFF) == 0xFF) {
        unsigned int dir = B(actor, 0x399);
        heading = func_80049398(actor, (dir << 7) & 0x80, 0x90000);
        if (angle_diff(heading, B(actor, 0x3AA)) < 0x20) {
            if (B(D_8010C1D8, 0x13) != 0 && B(Scratchpad, 0x30B) == 4) {
                func_8005E09C(actor, ((dir ^ 1) << 7) & 0x80, 0);
            }
            return;
        }
        if ((abs_s16(S(actor, 6)) >= 0x1E01 || abs_s16(S(actor, 2)) >= 0x3021)
            && W(actor, 0x3A0) != 0) {
            return;
        }
    }

    if (abs_s16(S(actor, 6)) < 0x1D00 && abs_s16(S(actor, 2)) < 0x2F20) {
        if (W(actor, 0x3A0) == 0) {
            heading = func_8005B7CC(heading & 0xFF, B(actor, 0x3AA), 0x20);
        } else if (func_800652EC(actor, 0x500) != 0) {
            if (D_800E6CB2 != 2) {
                unsigned char *ball = (unsigned char *)W(Scratchpad, 0x2F4);
                unsigned int dir = B(actor, 0x399);
                int ball_not_deep = (dir == 0) ? (S(ball, 2) < 0xBC1) : (S(ball, 2) >= -0xBC0);
                if (ball_not_deep && B(D_8010C1D8, 0x13) != 0) {
                    heading = func_8005B7CC(heading & 0xFF, B(actor, 0x3AA), 0x30);
                }
            }
        } else {
            heading = func_8005B7CC(heading & 0xFF, B(actor, 0x3AA), 0x20);
        }
    }

    func_8005E09C(actor, heading & 0xFF, dash_flag);
}
