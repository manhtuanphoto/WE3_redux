/*
 * func_800508C8
 *
 * Purpose:
 *   Human player cursor / active-player auto-switching logic. Checks player-switch
 *   button presses for sides 0 and 1, alternates processing between side 0 and
 *   side 1 on even/odd frames (Scratchpad+0x290 % 2), determines the ball/pass
 *   reference target position from Scratchpad+0x2B0, and selects the best
 *   outfield teammate based on distance to the target position, ball ownership,
 *   actor state, and offside/ahead-of-ball penalties.
 *
 * Arguments:
 *   None (void).
 *
 * Return value:
 *   None (void).
 *
 * Actor fields read/written:
 *   Candidate actors (10 consecutive 1000-byte structs from D_800FF748 + (base_id % 24) * 1000):
 *     Reads: +0x00 (active flag), +0x02 (x), +0x06 (z), +0x38D (actor id),
 *            +0x396 (state), +0x398 (side), +0x399 (attack dir),
 *            +0x39A (role/GK flag, 1=GK), +0x3D2 (mark/offside flag)
 *
 * Scratchpad fields read/written:
 *   Reads:  0x02E / 0x030 (switch button masks for side 0 / side 1),
 *           0x036 / 0x066 (ball x/z), 0x04A / 0x07A (predicted ball x/z),
 *           0x0FE / 0x100 (pass target x/z), 0x234 / 0x23C (pad 0/1 buttons),
 *           0x240[2] (active player actor id per side),
 *           0x242[2] (switch request mode per side: 0=auto, 1=auto-assist, 2=manual),
 *           0x244[2] (team base actor id per side),
 *           0x246[2] (switch cooldown frames per side),
 *           0x290 (frame counter), 0x29A (match phase; 1=open play),
 *           0x2A9 (ball owner id), 0x2B0 (ball event code 1..10),
 *           0x2B4 / 0x2B6 (target x/z, 0x7FFF = unset), 0x2B8 (0x7FFF = unset),
 *           0x2CF[2] (possession flags), 0x2E2 (mode), 0x2F2 (flag),
 *           0x2F4 (ball actor ptr; reads ball+0x3CC),
 *           0x307[2] (auto-switch disable flags), 0x31E[2] (GK actor id per atk_dir*2)
 *   Writes: 0x240[side] (new active player id),
 *           0x242[side] (cleared to 0, or temporarily 0xFF/2),
 *           0x246[side] (cooldown set to 2, 10, or 20 frames)
 *
 * Magic numbers / unit conversions (250 raw = 1 m, 30 frames/s):
 *   0x100    = 256 raw (1.024 m, ahead-of-target margin)
 *   0xA01    = 2561 raw (10.244 m, lateral z threshold)
 *   0xC01    = 3073 raw (12.292 m, lateral z threshold)
 *   0xFFFFF  = 1048575 (~1024^2 -> 4.096 m instant-keep threshold)
 *   0x100000 = 1048576 (switch-away penalty, equivalent to +4.096 m^2)
 *   0xFFF80000 = -524288 (bonus for states 3/15)
 *   0x900000 / 0x1440000 / 0x2400000 / 0x9000000 = positional/mark penalties
 *   Cooldowns: 2 frames (0.067 s), 10 frames (0.333 s), 20 frames (0.667 s)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800FF748[];

extern unsigned int func_80052654(unsigned int index);

void func_800508C8(void)
{
    int side_turn;
    unsigned char *base_actor;
    unsigned char *cand;
    unsigned char *best_actor = 0;
    int tx = 0;
    int tz = 0;
    unsigned int best_score;
    unsigned int score;
    unsigned int mode;
    unsigned int disable_flag;
    unsigned char base_id;
    int i;

    if ((H(Scratchpad, 0x234) & H(Scratchpad, 0x02E)) == H(Scratchpad, 0x02E)) {
        if ((func_80052654(B(Scratchpad, 0x244)) & 0xFF) == 0) {
            B(Scratchpad, 0x246) = 0;
            B(Scratchpad, 0x242) = 2;
        }
    }

    if ((H(Scratchpad, 0x23C) & H(Scratchpad, 0x030)) == H(Scratchpad, 0x030)) {
        if ((func_80052654(B(Scratchpad, 0x245)) & 0xFF) == 0) {
            B(Scratchpad, 0x247) = 0;
            B(Scratchpad, 0x243) = 2;
        }
    }

    side_turn = SW(Scratchpad, 0x290) % 2;

    if (B(Scratchpad, 0x246 + side_turn) != 0) {
        B(Scratchpad, 0x246 + side_turn)--;
        B(Scratchpad, 0x242 + side_turn) = 0xFF;
    }

    base_id = B(Scratchpad, 0x244 + side_turn);
    base_actor = D_800FF748 + ((unsigned int)base_id % 24U) * 1000;

    switch (W(Scratchpad, 0x2B0)) {
    case 1:
    {
        unsigned char *ball = (unsigned char *)W(Scratchpad, 0x2F4);
        if (S(ball, 0x3CC) < 2) {
            tx = H(Scratchpad, 0x036);
            tz = H(Scratchpad, 0x066);
        } else {
            tx = H(Scratchpad, 0x0FE);
            tz = H(Scratchpad, 0x100);
        }
        break;
    }
    case 2:
    case 8:
    case 10:
        if ((S(Scratchpad, 0x2B8) == 0x7FFF && B(Scratchpad, 0x2CF + B(base_actor, 0x398)) != 0) ||
            B(Scratchpad, 0x242 + side_turn) != 0) {
            if (S(Scratchpad, 0x2B4) == 0x7FFF) {
                tx = H(Scratchpad, 0x04A);
                tz = H(Scratchpad, 0x07A);
            } else {
                tx = H(Scratchpad, 0x2B4);
                tz = H(Scratchpad, 0x2B6);
            }
            if (B(Scratchpad, 0x2CF + B(base_actor, 0x398)) == 1) {
                tx = (B(base_actor, 0x399) != 0) ? ((signed short)tx + 0x100) : ((signed short)tx - 0x100);
            }
        } else {
            B(Scratchpad, 0x242 + side_turn) = 0xFF;
        }
        break;
    case 3:
        if (S(Scratchpad, 0x2B8) != 0x7FFF && B(Scratchpad, 0x242 + side_turn) == 0) {
            B(Scratchpad, 0x242 + side_turn) = 0xFF;
        } else {
            tx = H(Scratchpad, 0x04A);
            tz = H(Scratchpad, 0x07A);
        }
        break;
    default:
        if (S(Scratchpad, 0x2B4) == 0x7FFF) {
            tx = H(Scratchpad, 0x036);
            tz = H(Scratchpad, 0x066);
        } else {
            tx = H(Scratchpad, 0x2B4);
            tz = H(Scratchpad, 0x2B6);
        }
        break;
    }

    mode = B(Scratchpad, 0x242 + side_turn);
    if (mode == 0) {
        if ((B(Scratchpad, 0x2E2) & 0xFF) == 2) {
            disable_flag = B(Scratchpad, 0x307);
        } else {
            disable_flag = B(Scratchpad, 0x307 + side_turn);
        }

        if (B(Scratchpad, 0x29A) != 1 || disable_flag != 0) {
            B(Scratchpad, 0x242 + side_turn) = 0;
            return;
        }

        if (B(base_actor, 0x39A) == 1) {
            B(Scratchpad, 0x240 + side_turn) = B(Scratchpad, 0x31E + (B(base_actor, 0x399) << 1));
            B(Scratchpad, 0x242 + side_turn) = 0;
            return;
        }

        if (S(Scratchpad, 0x2B8) != 0x7FFF) {
            B(Scratchpad, 0x242 + side_turn) = 0;
            return;
        }

        best_score = 0xFFFFFFFFU;
        cand = base_actor;
        tx = (signed short)tx;
        tz = (signed short)tz;

        for (i = 0; i < 10; i++, cand += 1000) {
            unsigned char cand_id;
            unsigned char cand_state;
            int dx, dz;

            if (B(cand, 0x00) == 0) {
                continue;
            }
            cand_id = B(cand, 0x38D);
            if (cand_id == B(Scratchpad, 0x240 + (side_turn ^ 1))) {
                continue;
            }
            cand_state = B(cand, 0x396);
            if (cand_state == 0x11 || cand_state == 0x14) {
                continue;
            }
            if (cand_id == B(Scratchpad, 0x2A9)) {
                best_score = 0;
                best_actor = cand;
                break;
            }

            dx = S(cand, 0x02) - tx;
            dz = S(cand, 0x06) - tz;
            score = (unsigned int)(dx * dx + dz * dz);

            if (cand_id == B(Scratchpad, 0x240 + side_turn)) {
                if (score <= 0xFFFFFU) {
                    best_score = 0;
                    best_actor = cand;
                    break;
                }
                if (cand_state == 0x0B || cand_state == 0x1B) {
                    best_score = 0;
                    best_actor = cand;
                } else if (cand_state == 0x03 || cand_state == 0x0F) {
                    score += 0xFFF80000U;
                }
            } else {
                score += 0x100000U;
            }

            if (B(Scratchpad, 0x2CF + B(cand, 0x398)) == 1) {
                if (cand_id == B(Scratchpad, 0x240 + side_turn)) {
                    if (B(Scratchpad, 0x2A9) != 0 || W(Scratchpad, 0x2B0) == 1) {
                        int abs_dz = S(cand, 0x06) - tz;
                        if (abs_dz < 0) {
                            abs_dz = -abs_dz;
                        }
                        if (abs_dz >= 0xC01) {
                            score += 0x900000U;
                        } else {
                            int ref_x = (B(cand, 0x399) != 0) ? (-tx + 0x100) : (tx + 0x100);
                            int cx = (B(cand, 0x399) != 0) ? -S(cand, 0x02) : S(cand, 0x02);
                            if (ref_x < cx) {
                                score += 0x900000U;
                            }
                        }
                    }
                } else {
                    if (B(Scratchpad, 0x2A9) != 0 || W(Scratchpad, 0x2B0) == 1) {
                        int abs_dz = S(cand, 0x06) - tz;
                        if (abs_dz < 0) {
                            abs_dz = -abs_dz;
                        }
                        if (abs_dz >= 0xA01) {
                            score += 0x1440000U;
                        } else {
                            int ref_x = (B(cand, 0x399) != 0) ? (-tx + 0x100) : (tx + 0x100);
                            int cx = (B(cand, 0x399) != 0) ? -S(cand, 0x02) : S(cand, 0x02);
                            if (ref_x < cx) {
                                score += 0x1440000U;
                            }
                        }
                    }
                }
            } else if (B(cand, 0x3D2) != 0) {
                score += 0x9000000U;
            }

            if (score < best_score) {
                best_score = score;
                best_actor = cand;
            }
        }

        if (best_score != 0xFFFFFFFFU) {
            unsigned char best_id = B(best_actor, 0x38D);
            if (best_id != B(Scratchpad, 0x240 + side_turn)) {
                B(Scratchpad, 0x240 + side_turn) = best_id;
            }
        }
        B(Scratchpad, 0x246 + side_turn) = 2;
    } else if (mode == 1) {
        if (B(base_actor, 0x39A) == 1) {
            B(Scratchpad, 0x240 + side_turn) = B(Scratchpad, 0x31E + (B(base_actor, 0x399) << 1));
            B(Scratchpad, 0x242 + side_turn) = 0;
            return;
        }

        if ((B(Scratchpad, 0x2E2) & 0xFF) == 2) {
            disable_flag = B(Scratchpad, 0x307);
        } else {
            disable_flag = B(Scratchpad, 0x307 + side_turn);
        }
        if (disable_flag != 0) {
            B(Scratchpad, 0x242 + side_turn) = 0;
            return;
        }

        best_score = 0xFFFFFFFFU;
        cand = base_actor;
        tx = (signed short)tx;
        tz = (signed short)tz;

        for (i = 0; i < 10; i++, cand += 1000) {
            unsigned char cand_id;
            int dx, dz;

            if (B(cand, 0x00) == 0) {
                continue;
            }
            cand_id = B(cand, 0x38D);
            if (cand_id == B(Scratchpad, 0x240 + (side_turn ^ 1))) {
                continue;
            }

            dx = S(cand, 0x02) - tx;
            dz = S(cand, 0x06) - tz;
            score = (unsigned int)(dx * dx + dz * dz);

            if (cand_id == B(Scratchpad, 0x240 + side_turn)) {
                if (score <= 0xFFFFFU) {
                    best_score = 0;
                    best_actor = cand;
                    break;
                }
            } else {
                score += 0x100000U;
            }

            if (B(Scratchpad, 0x2F2) != 1 && B(cand, 0x3D2) != 0) {
                int abs_dz = S(cand, 0x06) - tz;
                if (abs_dz < 0) {
                    abs_dz = -abs_dz;
                }
                if (abs_dz >= 0xA01) {
                    score += 0x2400000U;
                } else {
                    int ref_x = (B(cand, 0x399) != 0) ? (-tx + 0x100) : (tx + 0x100);
                    int cx = (B(cand, 0x399) != 0) ? -S(cand, 0x02) : S(cand, 0x02);
                    if (ref_x < cx) {
                        score += 0x2400000U;
                    }
                }
            }

            if (score < best_score) {
                best_score = score;
                best_actor = cand;
            }
        }

        if (best_score != 0xFFFFFFFFU) {
            unsigned char best_id = B(best_actor, 0x38D);
            if (best_id != B(Scratchpad, 0x240 + side_turn)) {
                B(Scratchpad, 0x240 + side_turn) = best_id;
            }
        }
        B(Scratchpad, 0x246 + side_turn) = 0xA;
    } else if (mode == 2) {
        unsigned char ball_owner;

        if (B(base_actor, 0x39A) == 1) {
            B(Scratchpad, 0x240 + side_turn) = B(Scratchpad, 0x31E + (B(base_actor, 0x399) << 1));
            B(Scratchpad, 0x242 + side_turn) = 0;
            return;
        }

        best_score = 0xFFFFFFFFU;
        cand = base_actor;
        tx = (signed short)tx;
        tz = (signed short)tz;
        ball_owner = B(Scratchpad, 0x2A9);

        for (i = 0; i < 10; i++, cand += 1000) {
            unsigned char cand_id = B(cand, 0x38D);
            int dx, dz;

            if (cand_id == ball_owner && cand_id != B(Scratchpad, 0x240 + (side_turn ^ 1))) {
                best_score = 0;
                best_actor = cand;
                break;
            }
            if (B(cand, 0x00) == 0 ||
                cand_id == B(Scratchpad, 0x240) ||
                cand_id == B(Scratchpad, 0x241)) {
                continue;
            }

            dx = S(cand, 0x02) - tx;
            dz = S(cand, 0x06) - tz;
            score = (unsigned int)(dx * dx + dz * dz);

            if (B(Scratchpad, 0x2CF + B(cand, 0x398)) == 1 && B(Scratchpad, 0x2A9) != 0) {
                int ref_x = (B(cand, 0x399) != 0) ? (-tx + 0x100) : (tx + 0x100);
                int cx = (B(cand, 0x399) != 0) ? -S(cand, 0x02) : S(cand, 0x02);
                if (ref_x < cx) {
                    score += 0x900000U;
                }
            }

            if (score < best_score) {
                best_score = score;
                best_actor = cand;
            }
        }

        if (best_score != 0xFFFFFFFFU) {
            B(Scratchpad, 0x240 + side_turn) = B(best_actor, 0x38D);
        }
        B(Scratchpad, 0x246 + side_turn) = 0x14;
    }

    B(Scratchpad, 0x242 + side_turn) = 0;
}
