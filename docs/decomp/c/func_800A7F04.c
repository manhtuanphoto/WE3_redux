/*
 * func_800A7F04
 *
 * Purpose:
 *   Continuous sub-frame ball collision check against the goalposts, crossbar,
 *   and top/side exterior of the goal net. Interpolates 101 samples (0..100%)
 *   between the ball's previous position (+0x3C2/+0x3C4/+0x3C6) and current
 *   position (+0x02/+0x04/+0x06), triggering post/crossbar rebounds
 *   (func_800A7D58, func_800A7C2C), net-corner events (func_800987CC), or
 *   roof-of-net state transitions (SCRATCH+0xE0 = 2, func_80098968).
 *
 * Arguments:
 *   None
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Ball actor (ball = *(u8**)(SCRATCH+0x2F4)):
 *     +0x002 (s16) x position (R/W)
 *     +0x004 (s16) y height, negative = up (R/W)
 *     +0x006 (s16) z position (R/W)
 *     +0x00C (s32) y velocity (R/W)
 *     +0x3A0 (u32) speed << 8 (R/W)
 *     +0x3A4 (u8)  move heading, 0..255 = 360 deg (R/W)
 *     +0x3C2 (s16) previous frame x position (R)
 *     +0x3C4 (s16) previous frame y height (R/W)
 *     +0x3C6 (s16) previous frame z position (R/W)
 *     +0x3C8 (u16) contact/target flag (W)
 *   Scratchpad (0x1F800000):
 *     +0x0E0 (s16) goal net state (W: set to 2 on net roof fall)
 *     +0x2B0 (u32) ball event code (W: 0xD = woodwork/crossbar hit)
 *     +0x2F4 (u8*) pointer to ball actor (R)
 *     +0x2FC (u8)  disable/skip collision flag, 1 = skip (R)
 *     +0x31C (s16) ball timer, frames since last touch (W: set to 8)
 *   Globals:
 *     D_800CB690 (u8) woodwork cooldown timer in frames (R/W)
 *     D_800FF6A4 (u8) goal quadrant 1..4 (W)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x32FE = 13054 raw (52.216 m), 0x32FF = 13055 raw (52.220 m),
 *   0x3320 = 13088 raw (52.352 m, goal line), 0x3360 = 13152 raw (52.608 m),
 *   0x3361 = 13153 raw (52.612 m), 0x35C5 = 13765 raw (55.060 m) - X bounds
 *   0x372  = 882 raw (3.528 m, inner post edge), 0x391 = 913 raw (3.652 m),
 *   0x3A2  = 930 raw (3.720 m, goalpost center Z), 0x3B2 = 946 raw (3.784 m),
 *   0x3D2  = 978 raw (3.912 m, outer post edge) - Z bounds (post radius ~0.192 m)
 *   -0x246 = -582 raw (2.328 m, crossbar underside), -0x251 = -593 raw (2.372 m),
 *   -0x272 = -626 raw (2.504 m, crossbar center), -0x292 = -658 raw (2.632 m),
 *   -0x29E = -670 raw (2.680 m, top of net), -0x2A2 = -674 raw (2.696 m) - heights
 *   0x3E8  = 1000, 0x7D0 = 2000, 0xFA0 = 4000 - post-bounce speed boosts (<<8)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define SCRATCH ((unsigned char*)0x1F800000)

extern unsigned char D_800CB690;
extern unsigned char D_800FF6A4;

extern int func_8001E6B8(int n);
extern void func_800987CC(unsigned char *actor, int flag);
extern void func_80098968(void);
extern void func_800A7C2C(void);
extern void func_800A7D58(void);
extern void func_800AB620(int soundId);
extern int func_800AD678(void);

static void set_goal_quadrant(unsigned char *ball)
{
    if (S(ball, 2) > 0) {
        D_800FF6A4 = (S(ball, 6) > 0) ? 1 : 3;
    } else {
        D_800FF6A4 = (S(ball, 6) > 0) ? 2 : 4;
    }
}

static void crossbar_hit_clamp(unsigned char *ball)
{
    int x = S(ball, 2);
    int abs_x = (x < 0) ? -x : x;
    int z, prev_z, abs_z, abs_prev_z;

    if (abs_x >= 0x3361) {
        S(ball, 2) = (x >= 0) ? 0x3361 : -0x3361;
    }
    ball = (unsigned char *)W(SCRATCH, 0x2F4);
    x = S(ball, 2);
    abs_x = (x < 0) ? -x : x;
    if (abs_x < 0x3320) {
        S(ball, 2) = (x >= 0) ? 0x3320 : -0x3320;
    }

    func_800AB620(0x515);
    ball = (unsigned char *)W(SCRATCH, 0x2F4);
    W(SCRATCH, 0x2B0) = 0xD;
    D_800CB690 = 5;
    H(ball, 0x3C8) = 0;
    ball = (unsigned char *)W(SCRATCH, 0x2F4);
    H(SCRATCH, 0x31C) = 8;

    z = S(ball, 6);
    prev_z = S(ball, 0x3C6);
    abs_z = (z < 0) ? -z : z;
    abs_prev_z = (prev_z < 0) ? -prev_z : prev_z;
    if (abs_prev_z < abs_z && S(ball, 4) == -0x246) {
        S(ball, 2) = (S(ball, 2) >= 0) ? 0x3361 : -0x3361;
        ball = (unsigned char *)W(SCRATCH, 0x2F4);
        z = S(ball, 6);
        abs_z = (z < 0) ? -z : z;
        if (abs_z >= 0x373) {
            S(ball, 6) = (z >= 0) ? 0x372 : -0x3B2;
        }
    }
}

void func_800A7F04(void)
{
    unsigned char *ball;
    int bx, abs_bx;
    unsigned int ang_diff = 0;
    int toward_goal = 0;
    int prev_x, prev_y, prev_z;
    int cur_x, cur_y, cur_z;
    int i;

    if (B(SCRATCH, 0x2FC) == 1) {
        return;
    }
    if (D_800CB690 != 0) {
        D_800CB690--;
    }

    ball = (unsigned char *)W(SCRATCH, 0x2F4);
    bx = S(ball, 2);
    abs_bx = (bx < 0) ? -bx : bx;
    if (abs_bx < 0x32FF) {
        return;
    }

    /* Check if ball heading is within +-90 deg (0x40) of entering the nearest goal */
    if (bx > 0) {
        unsigned int ang = B(ball, 0x3A4);
        unsigned int neg = (-ang) & 0xFFU;
        if (neg < 0x81U) {
            ang_diff = neg;
        } else {
            ang_diff = ang;
        }
    } else {
        unsigned int ang = B(ball, 0x3A4);
        unsigned int diff = (0x80U - ang) & 0xFFU;
        if (diff < 0x81U) {
            ang_diff = diff;
        } else {
            ang_diff = (ang - 0x80U) & 0xFFU;
        }
    }
    if ((ang_diff & 0xFFU) < 0x40U) {
        toward_goal = 1;
    }
    if (D_800CB690 != 0) {
        toward_goal = 0;
    }

    ball = (unsigned char *)W(SCRATCH, 0x2F4);
    prev_x = S(ball, 0x3C2);
    prev_y = S(ball, 0x3C4);
    prev_z = S(ball, 0x3C6);
    cur_x = S(ball, 2);
    cur_y = S(ball, 4);
    cur_z = S(ball, 6);

    for (i = 0; i < 101; i++) {
        int px = (short)((prev_x * (100 - i) + cur_x * i) / 100);
        int py = (short)((prev_y * (100 - i) + cur_y * i) / 100);
        int pz = (short)((prev_z * (100 - i) + cur_z * i) / 100);
        int abs_px = (px < 0) ? -px : px;
        int abs_py, abs_pz;
        int diff_y;

        if (abs_px < 0x32FE) {
            continue;
        }

        abs_py = (py < 0) ? -py : py;
        if (abs_py < 0x29E) {
            if (toward_goal) {
                int diff_z;
                abs_pz = (pz < 0) ? -pz : pz;
                diff_z = 0x3A2 - abs_pz;
                if (diff_z < 0) {
                    diff_z = -diff_z;
                }
                if (diff_z < 0x20 - (int)((ang_diff & 0xFFU) >> 1) && abs_px < 0x3360) {
                    int z;
                    ball = (unsigned char *)W(SCRATCH, 0x2F4);
                    z = S(ball, 6);
                    S(ball, 6) = (short)((z >= 0 ? (z + 0x3A2) : (z - 0x3A2)) / 2);
                    func_800A7D58();
                    return;
                }

                abs_pz = (pz < 0) ? -pz : pz;
                if ((unsigned int)(abs_pz - 0x373) < 0x5FU) {
                    int bz, prev_bz, abs_bz, abs_prev_bz;
                    ball = (unsigned char *)W(SCRATCH, 0x2F4);
                    bz = S(ball, 6);
                    prev_bz = S(ball, 0x3C6);
                    abs_bz = (bz < 0) ? -bz : bz;
                    abs_prev_bz = (prev_bz < 0) ? -prev_bz : prev_bz;

                    if (0x3A2 - abs_pz < 0) {
                        if (abs_bz < abs_prev_bz) {
                            S(ball, 6) = (bz >= 0) ? 0x3D2 : -0x3D2;
                            goto post_rebound;
                        }
                        S(ball, 6) = (short)((bz >= 0 ? (bz + 0x3A2) : (bz - 0x3A2)) / 2);
                        func_800A7D58();
                        return;
                    } else {
                        if (abs_bz < abs_prev_bz) {
                            S(ball, 6) = (short)((bz >= 0 ? (bz + 0x3A2) : (bz - 0x3A2)) / 2);
                            func_800A7D58();
                            return;
                        }
                        if (abs_py < 0x247) {
                            S(ball, 6) = (bz >= 0) ? 0x372 : -0x372;
post_rebound:
                            func_800A7C2C();
                            ball = (unsigned char *)W(SCRATCH, 0x2F4);
                            bz = S(ball, 6);
                            abs_bz = (bz < 0) ? -bz : bz;
                            if (abs_bz == 0x372 && S(ball, 4) < S(ball, 0x3C4)) {
                                S(ball, 2) = (S(ball, 2) >= 0) ? 0x3361 : -0x3361;
                                ball = (unsigned char *)W(SCRATCH, 0x2F4);
                                if (S(ball, 4) < -0x246) {
                                    S(ball, 4) = -0x246;
                                }
                            }
                            return;
                        } else {
                            S(ball, 4) = (short)((S(ball, 4) - 0x246) / 2);
                            func_800A7D58();
                            return;
                        }
                    }
                }
            }

            /* Side post / net-corner grazing when past goal line */
            if (abs_px >= 0x3321) {
                int diff_z;
                abs_pz = (pz < 0) ? -pz : pz;
                diff_z = abs_pz - 0x3A2;
                if (diff_z < 0) {
                    diff_z = -diff_z;
                }
                if (diff_z < 0x20) {
                    int bz, prev_bz, abs_bz, abs_prev_bz;
                    ball = (unsigned char *)W(SCRATCH, 0x2F4);
                    bz = S(ball, 6);
                    prev_bz = S(ball, 0x3C6);
                    abs_bz = (bz < 0) ? -bz : bz;
                    abs_prev_bz = (prev_bz < 0) ? -prev_bz : prev_bz;
                    if (abs_bz < abs_prev_bz) {
                        if (abs_prev_bz == 0x372) {
                            return;
                        }
                        S(ball, 6) = (bz >= 0) ? 0x3D2 : -0x3D2;
                        ball = (unsigned char *)W(SCRATCH, 0x2F4);
                        bx = S(ball, 2);
                        abs_bx = (bx < 0) ? -bx : bx;
                        if (abs_bx < 0x3361) {
                            S(ball, 2) = (bx >= 0) ? 0x3361 : -0x3361;
                        }
                        ball = (unsigned char *)W(SCRATCH, 0x2F4);
                        set_goal_quadrant(ball);
                        func_800987CC((unsigned char *)W(SCRATCH, 0x2F4), 1);
                        return;
                    } else {
                        S(ball, 6) = (bz >= 0) ? 0x372 : -0x372;
                        ball = (unsigned char *)W(SCRATCH, 0x2F4);
                        bx = S(ball, 2);
                        abs_bx = (bx < 0) ? -bx : bx;
                        if (abs_bx < 0x3361) {
                            S(ball, 2) = (bx >= 0) ? 0x3361 : -0x3361;
                        }
                        ball = (unsigned char *)W(SCRATCH, 0x2F4);
                        set_goal_quadrant(ball);
                        func_800987CC((unsigned char *)W(SCRATCH, 0x2F4), 0);
                        return;
                    }
                }
            }
        }

        /* Crossbar and net roof checks */
        abs_pz = (pz < 0) ? -pz : pz;
        if (abs_pz >= 0x3D2 || D_800CB690 != 0) {
            continue;
        }

        diff_y = 0x272 - abs_py;
        if (diff_y < 0) {
            diff_y = -diff_y;
        }
        if (diff_y < 0x20 && abs_px < 0x3320) {
            ball = (unsigned char *)W(SCRATCH, 0x2F4);
            S(ball, 4) = (short)py;
            S(ball, 6) = (short)pz;
            func_800A7D58();
            return;
        }

        if (diff_y < 0x2A && abs_px < 0x3360) {
            ball = (unsigned char *)W(SCRATCH, 0x2F4);
            if (S(ball, 4) < S(ball, 0x3C4)) {
                int r, vy, bz, abs_bz;
                if (py < -0x251) {
                    S(ball, 4) = (short)py;
                    S(ball, 6) = (short)pz;
                    func_800A7D58();
                    return;
                }
                S(ball, 4) = -0x246;
                r = func_800AD678();
                ball = (unsigned char *)W(SCRATCH, 0x2F4);
                vy = SW(ball, 0xC);
                if (vy < 0) {
                    vy = -vy;
                }
                SW(ball, 0xC) = vy + ((r % 5) + 6) * 736;
                ball = (unsigned char *)W(SCRATCH, 0x2F4);
                W(ball, 0x3A0) = (W(ball, 0x3A0) * 6U) / 10U + 0x3E8U;
                ball = (unsigned char *)W(SCRATCH, 0x2F4);
                bz = S(ball, 6);
                abs_bz = (bz < 0) ? -bz : bz;
                if (abs_bz >= 0x373) {
                    S(ball, 6) = (bz >= 0) ? 0x372 : -0x3B2;
                }
                crossbar_hit_clamp(ball);
                return;
            } else {
                int vy, r;
                if (py >= -0x292) {
                    S(ball, 4) = (short)py;
                    S(ball, 6) = (short)pz;
                    func_800A7D58();
                    return;
                }
                S(ball, 4) = -0x2A2;
                ball = (unsigned char *)W(SCRATCH, 0x2F4);
                vy = (-SW(ball, 0xC) * 8) / 10 - 0x20;
                SW(ball, 0xC) = vy;
                r = func_8001E6B8(10);
                if (r == 5) {
                    int x;
                    ball = (unsigned char *)W(SCRATCH, 0x2F4);
                    B(ball, 0x3A4) = (unsigned char)(0x80 - B(ball, 0x3A4));
                    ball = (unsigned char *)W(SCRATCH, 0x2F4);
                    x = S(ball, 2);
                    S(ball, 2) = (short)((x >= 0 ? (px + 0x3320) : (px - 0x3320)) / 2);
                    ball = (unsigned char *)W(SCRATCH, 0x2F4);
                    W(ball, 0x3A0) = (W(ball, 0x3A0) >> 1) + 0xFA0U;
                    func_800AB620(0x515);
                    ball = (unsigned char *)W(SCRATCH, 0x2F4);
                    W(SCRATCH, 0x2B0) = 0xD;
                    D_800CB690 = 5;
                    H(ball, 0x3C8) = 0;
                    H(SCRATCH, 0x31C) = 8;
                    return;
                }
                ball = (unsigned char *)W(SCRATCH, 0x2F4);
                W(ball, 0x3A0) = (W(ball, 0x3A0) * 7U) / 10U + 0x7D0U;
                crossbar_hit_clamp(ball);
                return;
            }
        }

        /* Top of goal net behind goal line: 0x3321 <= abs(px) <= 0x35C4 */
        if ((unsigned int)(abs_px - 0x3321) < 0x2A4U && diff_y < 0x20) {
            ball = (unsigned char *)W(SCRATCH, 0x2F4);
            if (S(ball, 4) < S(ball, 0x3C4)) {
                int bz, prev_bz, abs_bz, abs_prev_bz;
                if (abs_py >= 0x273) {
                    S(ball, 4) = (short)py;
                    S(ball, 6) = (short)pz;
                    func_800A7D58();
                    return;
                }
                S(ball, 4) = -0x246;
                S(ball, 0x3C4) = -0x246;
                ball = (unsigned char *)W(SCRATCH, 0x2F4);
                bz = S(ball, 6);
                abs_bz = (bz < 0) ? -bz : bz;
                if (abs_bz >= 0x393) {
                    S(ball, 6) = (bz >= 0) ? 0x391 : -0x391;
                    ball = (unsigned char *)W(SCRATCH, 0x2F4);
                }
                prev_bz = S(ball, 0x3C6);
                abs_prev_bz = (prev_bz < 0) ? -prev_bz : prev_bz;
                if (abs_prev_bz >= 0x393) {
                    S(ball, 0x3C6) = (prev_bz >= 0) ? 0x391 : -0x391;
                }
                ball = (unsigned char *)W(SCRATCH, 0x2F4);
                bx = S(ball, 2);
                abs_bx = (bx < 0) ? -bx : bx;
                if (abs_bx < 0x3361) {
                    S(ball, 2) = (bx >= 0) ? 0x3361 : -0x3361;
                }
                return;
            } else {
                bx = S(ball, 2);
                abs_bx = (bx < 0) ? -bx : bx;
                if (abs_bx < 0x3361) {
                    S(ball, 2) = (bx >= 0) ? 0x3361 : -0x3361;
                }
                ball = (unsigned char *)W(SCRATCH, 0x2F4);
                /* UNCLEAR: +0x285 (positive, below ground) written before transitioning to net state 2 */
                S(ball, 4) = 0x285;
                S(SCRATCH, 0xE0) = 2;
                func_80098968();
                return;
            }
        }
    }
}
