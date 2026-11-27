/*
 * func_80053EB8 - Chaser-role handoff & double-team/contain check for defending/chasing actor.
 *
 * Purpose:
 *   Evaluates every 4 frames (when ball is free and ball timer >= 9 frames = 0.30 s)
 *   whether the current ball-chasing actor should:
 *   1) Hand off primary ball-chaser duty (scratch 0x31E[attack_dir]) to a better-positioned
 *      teammate (from 0x322/0x323[attack_dir]), putting the teammate into chase (state 0xF or 0x1B)
 *      and returning 0;
 *   2) Yield chasing if the nearest opponent (0x31E[attack_dir^1]) is closer to the intercept
 *      point and func_80052D90(opp) == 0, returning 0;
 *   3) Otherwise check if a second teammate / marked opponent (func_8005CE14) is close to the ball
 *      and trigger contain/jockey state (0x1B, substate 0x18), returning 1.
 *
 * Arguments:
 *   actor (u8*): player actor struct (1000 bytes).
 *
 * Return value:
 *   int: 0 if the actor handed off / yielded chase duty; 1 otherwise.
 *
 * Scratchpad reads/writes:
 *   - 0x1F800036/0x66 (s16, R): ball predicted X/Z at frame 0.
 *   - 0x1F800046/0x76 (s16, R): ball predicted X/Z at frame 8 (0.27 s).
 *   - 0x1F8000F8/0xFC (s16, R/W): intercept target X/Z (saved at entry and restored on exit).
 *   - 0x1F800290 (u32, R): global frame counter (runs only when frame & 3 == 0).
 *   - 0x1F8002A9 (u8, R): ball owner actor ID (0 = free).
 *   - 0x1F8002B4/0x2B6 (s16, R): ball predicted landing X/Z (0x7FFF = none).
 *   - 0x1F8002CF[2] (u8, R): per-side possession flag.
 *   - 0x1F8002F4 (u8*, R): pointer to ball actor.
 *   - 0x1F8002FC (u8, R): freeze/replay flag.
 *   - 0x1F80031C (s16, R): ball timer (frames since last touch).
 *   - 0x1F80031E[2]/0x31F[2] (u8, R/W): primary & secondary chaser actor IDs (stride 2 by attack dir).
 *   - 0x1F800322[2]/0x323[2] (u8, R): nearest & 2nd-nearest teammate actor IDs (stride 2 by attack dir).
 *
 * Actor fields reads/writes:
 *   - actor:    +0x02/+0x06 (s16 x/z, R), +0x38D (u8 id, R), +0x396 (u8 state, R/W),
 *               +0x397 (u8 substate, W), +0x398 (u8 side, R), +0x399 (u8 attack dir, R),
 *               +0x3A0 (u32 speed, R).
 *   - teammate: +0x02/+0x06 (s16 x/z, R), +0x38D (u8 id, R), +0x396 (u8 state, R/W),
 *               +0x397 (u8 substate, W), +0x3A0 (u32 speed, R), +0x3C2 (s16 target, W).
 *   - ball:     +0x02/+0x06 (s16 x/z, R), +0x3A4 (u8 move heading, R), +0x3B2 (u8, R),
 *               +0x3DC (s16, R).
 *
 * Magic numbers (250 raw = 1 m, 30 Hz):
 *   - 0x11 (17 frames) = 0.57 s; 9 frames = 0.30 s.
 *   - 0x40000 (512^2) -> 2.05 m; 0x90000 (768^2) -> 3.07 m; 0x3FFFF (~512^2) -> 2.05 m;
 *     0xFFFFF (~1024^2) -> 4.10 m; 0x100000 (1024^2) -> 4.10 m; 0x23FFFF (~1536^2) -> 6.14 m;
 *     0x400000 (2048^2) -> 8.19 m; 0x200 (512) -> 0.09 m^2 (very close intercept).
 *   - -0x23E1 (-9185) / -0x23E2 (-9186) = -36.74 m (own penalty area X boundary),
 *     0x12F4 (4852) = 19.41 m (penalty area Z half-width).
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))

extern unsigned char D_800FF748[];

extern void func_800676C4(int attack_dir, int target_x, int target_z, int a3);
extern void func_800484B8(int x, int z, int speed);
extern void func_800512B8(unsigned char *actor, int mode);
extern int func_80052C6C(unsigned char *actor);
extern int func_80052D90(unsigned char *actor);
extern int func_8005CE14(int actor_id);
extern unsigned int func_800494C0(unsigned char *actor);

int func_80053EB8(unsigned char *actor)
{
    unsigned char *scratch = (unsigned char *)0x1F800000;
    unsigned char *ball;
    unsigned char *mate;
    unsigned char *opp;
    unsigned char poss_flag;
    unsigned char attack_dir;
    signed short saved_f8, saved_fc;
    unsigned int actor_target_dist_sq;
    unsigned int opp_dist_sq;
    unsigned int mate_dist_sq;
    int dx, dz;

    poss_flag = B(scratch, 0x2CF + B(actor, 0x398));
    if (poss_flag == 1 && S(scratch, 0x31C) < 0x11) {
        return 1;
    }
    if (B(scratch, 0x2A9) != 0) {
        return 1;
    }
    if (S(scratch, 0x31C) < 9) {
        return 1;
    }
    ball = (unsigned char *)W(scratch, 0x2F4);
    if (B(ball, 0x3B2) != 0) {
        return 1;
    }
    if ((W(scratch, 0x290) & 3) != 0) {
        return 1;
    }

    saved_f8 = S(scratch, 0xF8);
    saved_fc = S(scratch, 0xFC);
    dx = S(actor, 2) - saved_f8;
    dz = S(actor, 6) - saved_fc;
    actor_target_dist_sq = (unsigned int)(dx * dx + dz * dz);

    if (poss_flag == 0) {
        int bdx = S(actor, 2) - S(scratch, 0x36);
        int bdz = S(actor, 6) - S(scratch, 0x66);
        if ((unsigned int)(bdx * bdx + bdz * bdz + 0x40000) >= actor_target_dist_sq) {
            return 1;
        }
    }

    if (S(scratch, 0x2B4) == 0x7FFF) {
        func_800676C4(B(actor, 0x399), S(scratch, 0x46), S(scratch, 0x76), 0);
    } else {
        func_800676C4(B(actor, 0x399), S(scratch, 0x2B4), S(scratch, 0x2B6), 0);
    }

    attack_dir = B(actor, 0x399);
    mate = D_800FF748 + (B(scratch, 0x322 + attack_dir * 2) % 24) * 1000;
    if (B(actor, 0x38D) == B(mate, 0x38D)) {
        mate = D_800FF748 + (B(scratch, 0x323 + attack_dir * 2) % 24) * 1000;
    }

    opp = D_800FF748 + (B(scratch, 0x31E + (attack_dir ^ 1) * 2) % 24) * 1000;
    func_800484B8(S(opp, 2), S(opp, 6), H(opp, 0x3A0));
    dx = S(opp, 2) - S(scratch, 0xF8);
    dz = S(opp, 6) - S(scratch, 0xFC);
    opp_dist_sq = (unsigned int)(dx * dx + dz * dz);

    if (B(actor, 0x38D) == B(scratch, 0x31E + B(actor, 0x399) * 2)) {
        int mate_in_own_box;

        func_800484B8(S(mate, 2), S(mate, 6), H(actor, 0x3A0));
        dx = S(mate, 2) - S(scratch, 0xF8);
        dz = S(mate, 6) - S(scratch, 0xFC);
        mate_dist_sq = (unsigned int)(dx * dx + dz * dz);

        if (mate_dist_sq + 0x90000 < actor_target_dist_sq) {
            int handoff = 0;
            int mate_behind_intercept = (B(actor, 0x399) != 0)
                ? (-S(mate, 2) < -S(scratch, 0xF8))
                : (S(mate, 2) < S(scratch, 0xF8));

            if (mate_dist_sq < opp_dist_sq || mate_behind_intercept) {
                if (opp_dist_sq < actor_target_dist_sq) {
                    handoff = 1;
                }
            }
            if (!handoff && mate_behind_intercept) {
                handoff = 1;
            }
            if (!handoff && actor_target_dist_sq <= 0x23FFFF) {
                unsigned char *b = (unsigned char *)W(scratch, 0x2F4);
                int dir_ang = (B(actor, 0x399) << 7) & 0xFF;
                int ball_ang = B(b, 0x3A4);
                int diff = (dir_ang - ball_ang) & 0xFF;
                int ang_close = (diff < 0x81)
                    ? (diff < 0x50)
                    : (((ball_ang - dir_ang) & 0xFF) < 0x50);

                if (!ang_close) {
                    int target_behind_actor = (B(actor, 0x399) != 0)
                        ? (-saved_f8 < -S(actor, 2))
                        : (saved_f8 < S(actor, 2));
                    if (target_behind_actor) {
                        handoff = 1;
                    }
                }
            }

            if (handoff) {
                B(scratch, 0x31E + B(actor, 0x399) * 2) = B(mate, 0x38D);
                B(scratch, 0x31F + B(actor, 0x399) * 2) = B(actor, 0x38D);
                func_800512B8(mate, 1);
                S(scratch, 0xF8) = saved_f8;
                S(scratch, 0xFC) = saved_fc;
                if ((func_80052C6C(mate) & 0xFF) != 0) {
                    return 1;
                }
                if (B(scratch, 0x2A9) == 0 &&
                    (B(actor, 0x396) == 0x1B || B(actor, 0x396) == 1)) {
                    B(mate, 0x396) = 0x1B;
                    B(mate, 0x397) = 0;
                    S(mate, 0x3C2) = 0xFF;
                } else {
                    B(mate, 0x396) = 0xF;
                    B(mate, 0x397) = 0;
                }
                return 0;
            }
        }

        mate_in_own_box = (B(actor, 0x399) != 0)
            ? (-S(mate, 2) < -0x23E1)
            : (S(mate, 2) < -0x23E1);
        if (mate_in_own_box) {
            int abs_z = S(mate, 6);
            if (abs_z < 0) {
                abs_z = -abs_z;
            }
            if (abs_z < 0x12F4) {
                int skip_retreat = 0;
                if (B(scratch, 0x2A9) == 0) {
                    if (S(scratch, 0x2B4) != 0x7FFF) {
                        goto check_contain;
                    }
                    if (actor_target_dist_sq <= 0x23FFFF && actor_target_dist_sq < opp_dist_sq) {
                        unsigned char *b = (unsigned char *)W(scratch, 0x2F4);
                        int bdx = S(actor, 2) - S(b, 2);
                        int bdz = S(actor, 6) - S(b, 6);
                        if (bdx * bdx + bdz * bdz > 0x3FFFF) {
                            skip_retreat = 1;
                        }
                    } else {
                        skip_retreat = 1;
                    }
                }
                if (!skip_retreat) {
                    if ((func_80052D90(mate) & 0xFF) == 0) {
                        unsigned char st = B(mate, 0x396);
                        if (st != 7 && st != 0x17 && st != 0) {
                            B(mate, 0x396) = 1;
                            B(mate, 0x397) = 0;
                        }
                    }
                }
            }
        }
    }

    if (S(scratch, 0x2B4) == 0x7FFF) {
        if (B(actor, 0x396) != 0xF) {
            if (opp_dist_sq < 0x200 || actor_target_dist_sq <= 0xFFFFF) {
                if ((func_80052D90(opp) & 0xFF) == 0 && opp_dist_sq < actor_target_dist_sq) {
                    S(scratch, 0xF8) = saved_f8;
                    S(scratch, 0xFC) = saved_fc;
                    return 0;
                }
            }
        }
    }

check_contain:
    if (actor_target_dist_sq > 0x23FFFF) {
        S(scratch, 0xF8) = saved_f8;
        S(scratch, 0xFC) = saved_fc;
        return 1;
    }

    {
        int actor_deep = (B(actor, 0x399) != 0)
            ? (-S(actor, 2) < -0x23E2)
            : (S(actor, 2) < -0x23E2);
        if (!actor_deep) {
            S(scratch, 0xF8) = saved_f8;
            S(scratch, 0xFC) = saved_fc;
            return 1;
        }
    }

    {
        int mark_id = func_8005CE14(B(actor, 0x38D)) & 0xFF;
        unsigned char *marked = D_800FF748 + (mark_id % 24) * 1000;
        unsigned char *b;
        int bdx, bdz;

        if (B(actor, 0x38D) == B(marked, 0x38D)) {
            S(scratch, 0xF8) = saved_f8;
            S(scratch, 0xFC) = saved_fc;
            return 1;
        }

        b = (unsigned char *)W(scratch, 0x2F4);
        bdx = S(marked, 2) - S(b, 2);
        bdz = S(marked, 6) - S(b, 6);
        if (bdx * bdx + bdz * bdz > 0x400000) {
            S(scratch, 0xF8) = saved_f8;
            S(scratch, 0xFC) = saved_fc;
            return 1;
        }

        func_800484B8(S(marked, 2), S(marked, 6), H(marked, 0x3A0));
        dx = S(marked, 2) - S(scratch, 0xF8);
        dz = S(marked, 6) - S(scratch, 0xFC);
        mate_dist_sq = (unsigned int)(dx * dx + dz * dz);

        if (B(marked, 0x396) == 0x9D && mate_dist_sq <= 0x3FFFF) {
            b = (unsigned char *)W(scratch, 0x2F4);
            if (S(b, 0x3DC) != 0 || func_800494C0(marked) > 0x100000) {
                goto enter_contain;
            }
        }

        if (B(scratch, 0x2FC) != 0) {
            goto enter_contain;
        }

        if (mate_dist_sq < actor_target_dist_sq) {
            goto enter_contain;
        }

        if (mate_dist_sq > 0x3FFFF) {
            func_800512B8(actor, 1);
            S(scratch, 0xF8) = saved_f8;
            S(scratch, 0xFC) = saved_fc;
            return 1;
        }
    }

enter_contain:
    S(scratch, 0xF8) = saved_f8;
    S(scratch, 0xFC) = saved_fc;
    B(actor, 0x396) = 0x1B;
    B(actor, 0x397) = 0x18;
    return 1;
}
