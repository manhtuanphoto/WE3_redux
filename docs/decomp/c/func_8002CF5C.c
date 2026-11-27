/*
 * func_8002CF5C
 *
 * Purpose:
 *   Computes and refines the forward/backward lead-x (and occasional z-jitter)
 *   pass target offset in actor+0x3C6 / actor+0x3C8 for a player about to pass
 *   (actor+0x3C2 = target actor id, actor+0x3C4 = pass mode: 0 = forward/normal,
 *   1 = through/ahead lead, 4 = fixed-base lead). Checks opponent pressure along
 *   the pass lane and adjusts or caps the lead distance based on field bounds,
 *   body orientation, and opponent interception timing.
 *
 * Arguments:
 *   actor - pointer to the passing player's 1000-byte actor struct.
 *
 * Return value:
 *   None (void).
 *
 * Actor fields read/written:
 *   Reads:  +0x02 (x), +0x06 (z), +0x399 (attack dir), +0x3AA (body heading),
 *           +0x3B0 (ball-control/state flag), +0x3C2 (target actor id),
 *           +0x3C4 (pass mode: 0, 1, 4)
 *   Writes: +0x3C6 (s16 x lead offset), +0x3C8 (s16 z lead offset, modes 1/4)
 *   Target actor (D_800FF748 + (target_id % 24) * 1000):
 *     Reads: +0x02 (x), +0x06 (z), +0x08 (vx), +0x10 (vz),
 *            +0x3B4 (max speed), +0x3D2 (offside/mark flag)
 *   Nearest opponent actor (D_800FF748 + (opp_id % 24) * 1000):
 *     Reads: +0x02 (x), +0x06 (z)
 *
 * Scratchpad fields read/written:
 *   Reads:  0x2E1 (game/match mode; 5 skips opponent check in modes 1/4),
 *           0x322[(atk_dir ^ 1) * 2] (nearest opponent id set by func_8004968C)
 *
 * Magic numbers / unit conversions (250 raw = 1 m, 30 frames/s):
 *   0x200  = 512 raw (2.048 m, lead step increment)
 *   0x400  = 1024 raw (4.096 m, max mode-0 lead)
 *   0x600  = 1536 raw (6.144 m, min mode-1/4 lead)
 *   0x1800 = 6144 raw (24.576 m, max mode-1/4 lead)
 *   0xBB6  = 2998 raw (11.992 m, penalty-box z half-width check)
 *   0x12F4 = 4852 raw (19.408 m), 0x1B00 = 6912 raw (27.648 m)
 *   0x1990 = 6544 raw (26.176 m), 0x1FE2 = 8162 raw (32.648 m)
 *   0x23E3 = 9187 raw (36.748 m), 0x24E2 = 9442 raw (37.768 m)
 *   0x2B20 = 11040 raw (44.160 m, near-touchline/goal-line x limit)
 *   0x190000 = 1638400 (1280^2 -> 5.12 m distance threshold)
 *   0x23FFFF / 0x3FFFFF = squared-distance thresholds (~6.14 m / ~8.19 m)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800FF748[];

extern int func_8005B36C(int x1, int z1, int x2, int z2);
extern int func_800533F4(unsigned char *actor, unsigned int angle, int arg2, unsigned int dist_sq);
extern unsigned int func_8004968C(unsigned char *actor, int x, int z);
extern unsigned int func_800B1128(unsigned int val);
extern int func_8001E6B8(int n);

void func_8002CF5C(unsigned char *actor)
{
    signed short target_id;
    unsigned char *target;
    unsigned char *opp;
    signed short pass_mode;
    int pass_angle;
    unsigned int target_dist_sq;
    unsigned int dist_sq;
    unsigned int lead_dist;
    unsigned int best_opp_dist;
    int lead;
    int next_lead;
    int step_idx;
    int cand_x;
    int abs_x;
    int diff;
    int abs_diff;
    int sector;
    unsigned int opp_dist;
    int opp_id;
    int time_margin;
    int ang;
    int base_x;

    target_id = S(actor, 0x3C2);
    if (target_id == 0) {
        return;
    }

    S(actor, 0x3C6) = 0;
    target = D_800FF748 + (signed short)(target_id % 24) * 1000;
    pass_mode = S(actor, 0x3C4);

    if (pass_mode == 0) {
        int pred_x = (signed short)(H(target, 0x02) + (SW(target, 0x08) << 2));
        int pred_z = (signed short)(H(target, 0x06) + (SW(target, 0x10) << 2));
        int dx, dz;

        pass_angle = func_8005B36C(S(actor, 0x02), S(actor, 0x06), pred_x, pred_z);
        dx = S(actor, 0x02) - S(target, 0x02);
        dz = S(actor, 0x06) - S(target, 0x06);
        target_dist_sq = (unsigned int)(dx * dx + dz * dz);

        if ((func_800533F4(actor, pass_angle & 0xFF, 0x10, target_dist_sq) & 0xFF) != 0) {
            return;
        }

        lead = 0x200;
        best_opp_dist = 0;
        while (lead <= 0x400) {
            cand_x = (B(actor, 0x399) != 0) ? (S(target, 0x02) - lead) : (S(target, 0x02) + lead);
            cand_x = (signed short)cand_x;
            abs_x = (cand_x < 0) ? -cand_x : cand_x;
            if (abs_x >= 0x2B21) {
                break;
            }

            opp_dist = func_800B1128(func_8004968C(actor, cand_x, S(target, 0x06)));
            S(actor, 0x3C6) = (signed short)lead;
            if ((unsigned int)(lead / 48 + 8) >= opp_dist / 66U) {
                break;
            }
            lead += 0x200;
            best_opp_dist = opp_dist;
        }

        cand_x = (B(actor, 0x399) != 0) ? (S(target, 0x02) - S(actor, 0x3C6)) : (S(target, 0x02) + S(actor, 0x3C6));
        cand_x = (signed short)cand_x;
        dx = S(actor, 0x02) - cand_x;
        dz = S(actor, 0x06) - S(target, 0x06);
        lead_dist = func_800B1128((unsigned int)(dx * dx + dz * dz));

        diff = (pass_angle - B(actor, 0x3AA)) & 0xFF;
        if (diff < 0x81) {
            abs_diff = diff;
        } else {
            abs_diff = (B(actor, 0x3AA) - pass_angle) & 0xFF;
        }
        if (abs_diff >= 0x50 || B(actor, 0x3B0) == 0) {
            S(actor, 0x3C6) = 0;
            return;
        }

        base_x = B(actor, 0x399) << 7;
        diff = (pass_angle - base_x) & 0xFF;
        if (diff < 0x81) {
            sector = (diff + 4) & 0xFF;
        } else {
            sector = ((base_x - pass_angle) + 4) & 0xFF;
        }
        sector >>= 3;

        switch (sector) {
        case 0:
            S(actor, 0x3C6) = 0;
            return;

        case 1:
        case 2:
        case 3:
            if (B(target, 0x3D2) != 0) {
                if (target_dist_sq >= 0xC01U) {
                    S(actor, 0x3C6) = 0;
                }
            }
            /* fall through */
        case 4:
        case 5:
        case 6:
        default:
            if (lead_dist < 0x400U && S(actor, 0x3C6) >= 0x201) {
                S(actor, 0x3C6) = 0x200;
            }
            return;

        case 7:
        case 8:
        case 9:
            diff = (base_x - B(actor, 0x3AA)) & 0xFF;
            abs_diff = (diff < 0x81) ? diff : ((B(actor, 0x3AA) - base_x) & 0xFF);
            if (abs_diff < 0x31) {
                return;
            }
            diff = (pass_angle - B(actor, 0x3AA)) & 0xFF;
            abs_diff = (diff < 0x81) ? diff : ((B(actor, 0x3AA) - pass_angle) & 0xFF);
            if (abs_diff < 0x31) {
                if (lead_dist < 0x600U) {
                    if (S(actor, 0x3C6) >= 0x101) {
                        S(actor, 0x3C6) = 0x100;
                    }
                }
                return;
            }
            /* fall through */
        case 10:
        case 11:
            diff = ((B(actor, 0x399) << 7) - B(actor, 0x3AA)) & 0xFF;
            abs_diff = (diff < 0x81) ? diff : ((B(actor, 0x3AA) - (B(actor, 0x399) << 7)) & 0xFF);
            if (abs_diff < 0x31) {
                return;
            }
            cand_x = (B(actor, 0x399) != 0) ? -S(target, 0x02) : S(target, 0x02);
            if (cand_x < 0x1FE2 || best_opp_dist < 0x400U || target_dist_sq <= 0x23FFFFU) {
                if (S(actor, 0x3C6) >= 0x101) {
                    S(actor, 0x3C6) = 0x100;
                }
                return;
            }
            if (S(actor, 0x3C6) >= 0x401) {
                S(actor, 0x3C6) = 0x400;
            }
            if ((lead_dist >> 1) < (unsigned int)S(actor, 0x3C6)) {
                S(actor, 0x3C6) = (signed short)(lead_dist >> 1);
            }
            return;

        case 12:
        case 13:
            diff = (base_x - B(actor, 0x3AA)) & 0xFF;
            abs_diff = (diff < 0x81) ? diff : ((B(actor, 0x3AA) - base_x) & 0xFF);
            if (abs_diff < 0x31) {
                return;
            }
            /* fall through */
        case 14:
        case 15:
        case 16:
            cand_x = (B(actor, 0x399) != 0) ? -S(actor, 0x02) : S(actor, 0x02);
            if (cand_x < 0x1990 || target_dist_sq <= 0x3FFFFFU) {
                if (S(actor, 0x3C6) >= 0x101) {
                    S(actor, 0x3C6) = 0x100;
                }
                return;
            }
            if (S(actor, 0x3C6) >= 0x801) {
                S(actor, 0x3C6) = (signed short)((S(actor, 0x3C6) - 0x800) >> 1);
            }
            return;
        }
    } else if (pass_mode == 1 || pass_mode == 4) {
        int dx, dz;
        int min_lead;

        pass_angle = func_8005B36C(S(actor, 0x02), S(actor, 0x06), S(target, 0x02), S(target, 0x06));
        base_x = (B(actor, 0x399) ^ 1) << 7;
        if (((pass_angle - base_x) & 0xFF) < 0x81U) {
            lead = func_8005B36C(S(actor, 0x02), S(actor, 0x06), S(target, 0x02), S(target, 0x06))
                   - ((B(actor, 0x399) ^ 1) << 7);
        } else {
            lead = ((B(actor, 0x399) ^ 1) << 7)
                   - func_8005B36C(S(actor, 0x02), S(actor, 0x06), S(target, 0x02), S(target, 0x06));
        }

        dx = S(actor, 0x02) - S(target, 0x02);
        dz = S(actor, 0x06) - S(target, 0x06);
        opp_dist = func_800B1128((unsigned int)(dx * dx + dz * dz));

        if (S(actor, 0x3C4) == 4) {
            min_lead = 0x600;
        } else {
            min_lead = (int)(opp_dist >> 2) + ((lead & 0xFF) << 3) + 0x100;
            if ((unsigned int)min_lead < 0x601U) {
                min_lead = 0x600;
            }
        }
        lead = min_lead;
        step_idx = 0;
        S(actor, 0x3C6) = (signed short)min_lead;
        S(actor, 0x3C8) = 0;

        if (min_lead < 0x1801) {
            next_lead = min_lead + 0x200;
            while (lead < 0x1801) {
                int pred_tx = S(target, 0x02) + SW(target, 0x08) * 3;
                int abs_tz = S(target, 0x06);

                cand_x = (B(actor, 0x399) != 0) ? (pred_tx - lead) : (pred_tx + lead);
                if (abs_tz < 0) {
                    abs_tz = -abs_tz;
                }
                if (abs_tz < 0xBB6) {
                    abs_x = (signed short)cand_x;
                    if (abs_x < 0) {
                        abs_x = -abs_x;
                    }
                    if (abs_x >= 0x24E3) {
                        int abs_pred = (pred_tx < 0) ? -pred_tx : pred_tx;
                        S(actor, 0x3C6) = (signed short)(0x24E2 - abs_pred);
                        break;
                    }
                } else {
                    abs_x = (signed short)cand_x;
                    if (abs_x < 0) {
                        abs_x = -abs_x;
                    }
                    if (abs_x >= 0x2B21) {
                        int abs_pred = (pred_tx < 0) ? -pred_tx : pred_tx;
                        S(actor, 0x3C6) = (signed short)(0x2B20 - abs_pred);
                        break;
                    }
                }

                if ((B(Scratchpad, 0x2E1) & 0xFF) == 5) {
                    break;
                }

                opp_dist = func_800B1128(func_8004968C(actor, (signed short)cand_x, S(target, 0x06)));
                {
                    unsigned char atk_dir = B(actor, 0x399);
                    opp_id = B(Scratchpad, 0x322 + ((atk_dir ^ 1) << 1));
                    opp = D_800FF748 + (unsigned int)(opp_id % 24) * 1000;
                    time_margin = (int)(opp_dist >> 6) - (lead / B(target, 0x3B4) + step_idx);

                    if (time_margin >= 0) {
                        S(actor, 0x3C6) = (signed short)lead;
                    } else if (step_idx < 2) {
                        int tx = S(target, 0x02);
                        int tz = S(target, 0x06);
                        int az = S(actor, 0x06);
                        dx = S(actor, 0x02) - tx;
                        dz = az - tz;
                        dist_sq = (unsigned int)(dx * dx + dz * dz);
                        if ((int)dist_sq > 0x190000 && ((tz < 0) ? -tz : tz) < 0x1B00) {
                            if (az < tz) {
                                if (tz >= 0x12F5) {
                                    int check_x = (atk_dir != 0) ? -tx : tx;
                                    if (check_x >= 0x23E3) {
                                        S(actor, 0x3C8) = 0x180;
                                    } else {
                                        S(actor, 0x3C6) = (signed short)next_lead;
                                        S(actor, 0x3C8) = (signed short)(func_8001E6B8(0x100) - (H(target, 0x06) - 0x1B00));
                                    }
                                }
                            } else {
                                if (tz < -0x12F4) {
                                    int check_x = (atk_dir != 0) ? -tx : tx;
                                    if (check_x >= 0x23E3) {
                                        S(actor, 0x3C8) = -0x180;
                                    } else {
                                        S(actor, 0x3C6) = (signed short)next_lead;
                                        S(actor, 0x3C8) = (signed short)((-0x1B00 - H(target, 0x06)) - func_8001E6B8(0x100));
                                    }
                                }
                            }
                            break;
                        }
                        goto check_intercept;
                    } else {
                    check_intercept:
                        if (step_idx == 0 && S(actor, 0x3C4) == 1 && time_margin < -2) {
                            int tgt_x2 = (signed short)(B(actor, 0x399) != 0 ? (S(target, 0x02) - 0x800) : (S(target, 0x02) + 0x800));
                            ang = func_8005B36C(S(opp, 0x02), S(opp, 0x06), tgt_x2, S(target, 0x06));
                            if (((ang - pass_angle) & 0xFF) < 0x81U) {
                                int tgt_x3 = (signed short)(B(actor, 0x399) != 0 ? (S(target, 0x02) - 0x800) : (S(target, 0x02) + 0x800));
                                ang = func_8005B36C(S(opp, 0x02), S(opp, 0x06), tgt_x3, S(target, 0x06));
                                diff = (ang - pass_angle) & 0xFF;
                            } else {
                                int tgt_x3 = (signed short)(B(actor, 0x399) != 0 ? (S(target, 0x02) - 0x800) : (S(target, 0x02) + 0x800));
                                ang = func_8005B36C(S(opp, 0x02), S(opp, 0x06), tgt_x3, S(target, 0x06));
                                diff = (pass_angle - ang) & 0xFF;
                            }
                            if (diff < 0x41) {
                                S(actor, 0x3C6) = (signed short)next_lead;
                                step_idx = -1;
                            } else {
                                break;
                            }
                        } else {
                            break;
                        }
                    }
                }

                next_lead += 0x200;
                lead += 0x200;
                step_idx++;
            }
        }

        if ((unsigned int)S(actor, 0x3C6) < (unsigned int)min_lead || S(actor, 0x3C6) < 0) {
            S(actor, 0x3C6) = (signed short)min_lead;
        }
    } else {
        S(actor, 0x3C6) = 0;
    }
}
