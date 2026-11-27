/*
 * func_800A27C0
 *
 * Purpose:
 *   Coordinates CPU off-ball attacking support runs during open play when the
 *   team in possession has crossed into the opponent's half. Depending on team
 *   tactic (D_8010C1D8[side + 0xD]) and ball lateral position, selects a winger,
 *   forward, or midfielder to trigger an overlapping/support run by writing
 *   actor+0x3E2 (8, 9, or 10) and clearing actor+0x3E3.
 *
 * Arguments:
 *   None (void).
 *
 * Return value:
 *   None (void).
 *
 * Actor fields read/written:
 *   Reads:  +0x00 (active), +0x06 (z), +0x38D (actor id), +0x391 (role id),
 *           +0x392 (team slot), +0x398 (side 0/1), +0x399 (attack dir)
 *   Writes: +0x3E2 (8 = right/top wing run, 9 = left/bottom wing run, 10 = center run),
 *           +0x3E3 (= 0)
 *   Ball actor (Scratchpad + 0x2F4):
 *     Reads: +0x02 (x), +0x06 (z)
 *
 * Scratchpad fields read/written:
 *   Reads:  0x06 (cand wing A id), 0x07 (cand wing B id), 0x08 (cand center id),
 *           0x290 (frame counter), 0x29A (match phase; must be 1),
 *           0x2A9 (ball owner id), 0x2CF[side] (possession flags),
 *           0x2F4 (ball actor ptr), 0x2FF[side], 0x31E[atk_dir * 2]
 *   Writes: 0x06, 0x07, 0x08 (candidate actor ids cleared/selected)
 *
 * Magic numbers / unit conversions (250 raw = 1 m, 30 frames/s):
 *   300 frames = 10.0 s interval to randomize D_800CB650 (0 or 1)
 *   0x3E8 = 1000 bytes (actor struct stride; 0x7D0 = actor[2], 0x32C8 = actor[13],
 *           0x2AF8 = actor[11], 0x55F0 = actor[22])
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned char D_8010C1D8[];
extern unsigned char D_8010C32B[];
extern unsigned char D_800CB650;
extern unsigned char D_801002B0;
extern unsigned char D_80102DA8;

extern int func_8001E6B8(int n);

void func_800A27C0(void)
{
    int match_phase;
    unsigned char *team_actor;
    unsigned char *ball;
    unsigned char atk_dir;
    unsigned char tactic;
    int run_mode;
    int i;

    match_phase = B(Scratchpad, 0x29A);
    if (match_phase != 1) {
        return;
    }

    if (SW(Scratchpad, 0x290) % 300 == 0) {
        D_800CB650 = (unsigned char)func_8001E6B8(2);
    }

    B(Scratchpad, 0x08) = 0;
    B(Scratchpad, 0x06) = 0;
    B(Scratchpad, 0x07) = 0;

    if (B(Scratchpad, 0x2CF + D_801002B0) == 0) {
        team_actor = D_800FF748 + 0x7D0;
    } else if (B(Scratchpad, 0x2CF + D_80102DA8) != 0) {
        return;
    } else {
        team_actor = D_800FF748 + 0x32C8;
    }

    atk_dir = B(team_actor, 0x399);
    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    if (atk_dir == 0) {
        if (S(ball, 0x02) <= 0) {
            if (atk_dir != match_phase || S(ball, 0x02) >= 0) {
                return;
            }
        }
    } else {
        if (atk_dir != match_phase || S(ball, 0x02) >= 0) {
            return;
        }
    }

    tactic = B(D_8010C1D8, B(team_actor, 0x398) + 0xD);
    run_mode = tactic;

    if (tactic >= 2 && tactic < 4) {
        goto select_wing_runners;
    }

    if (tactic == 4) {
        int best_score = 0;
        unsigned char *p;

        p = (B(Scratchpad, 0x2CF + B(D_800FF748, 0xB68)) == 0) ? (D_800FF748 + 0x7D0) : (D_800FF748 + 0x32C8);
        for (i = 0; i < 10; i++, p += 0x3E8) {
            if (B(p, 0x391) == 0x19 && B(p, 0x00) != 0) {
                unsigned char id = B(p, 0x38D);
                unsigned char *chosen = D_800FF748 + (id % 24) * 1000;
                B(Scratchpad, 0x08) = id;
                B(chosen, 0x3E2) = 0xA;
                B(chosen, 0x3E3) = 0;
                return;
            }
        }

        p = (B(Scratchpad, 0x2CF + B(D_800FF748, 0xB68)) == 0) ? (D_800FF748 + 0x7D0) : (D_800FF748 + 0x32C8);
        for (i = 0; i < 10; i++, p += 0x3E8) {
            unsigned char role = B(p, 0x391);
            if (((unsigned int)(role - 0x16) < 2U) || (role == 0x15 && B(p, 0x00) != 0)) {
                int off = B(p, 0x392) * 12 + B(p, 0x398) * 264;
                int score = (B(D_8010C32B, off) & 0xF) * 10;
                if (best_score < score) {
                    best_score = score;
                    B(Scratchpad, 0x08) = B(p, 0x38D);
                }
            }
        }

        if (B(Scratchpad, 0x08) == 0) {
            /* UNCLEAR: p is not reset before this loop in the disassembly, continuing
             * from the end of the previous 10-actor scan. */
            for (i = 0; i < 10; i++, p += 0x3E8) {
                unsigned char role = B(p, 0x391);
                if (role == 0x12 || (role == 0x11 && B(p, 0x00) != 0)) {
                    int off = B(p, 0x392) * 12 + B(p, 0x398) * 264;
                    int score = (B(D_8010C32B, off) & 0xF) * 10;
                    if (best_score < score) {
                        best_score = score;
                        B(Scratchpad, 0x08) = B(p, 0x38D);
                    }
                }
            }
        }

        {
            unsigned char id = B(Scratchpad, 0x08);
            unsigned char *chosen = D_800FF748 + (id % 24) * 1000;
            B(chosen, 0x3E2) = 0xA;
            B(chosen, 0x3E3) = 0;
        }
        return;
    }

    atk_dir = B(team_actor, 0x399);
    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    if (atk_dir == 0) {
        if (S(ball, 0x02) <= 0) {
            if (atk_dir != 1 || S(ball, 0x02) >= 0) {
                return;
            }
        }
    } else {
        if (atk_dir != 1 || S(ball, 0x02) >= 0) {
            return;
        }
    }

    if (B(team_actor, 0x399) != 0) {
        run_mode = (S(ball, 0x06) <= 0) ? 3 : 2;
    } else {
        run_mode = (S(ball, 0x06) <= 0) ? 2 : 3;
    }

select_wing_runners:
    {
        unsigned char *p;
        unsigned char owner = B(Scratchpad, 0x2A9);

        p = (B(Scratchpad, 0x2CF + B(D_800FF748, 0xB68)) == 0) ? (D_800FF748 + 0x7D0) : (D_800FF748 + 0x32C8);
        for (i = 0; i < 10; i++, p += 0x3E8) {
            unsigned char id = B(p, 0x38D);
            if (id == owner || B(p, 0x00) == 0 || id == B(Scratchpad, 0x31E + (B(p, 0x399) << 1))) {
                continue;
            }
            {
                int role = B(p, 0x391);
                if (role == 0xC) {
                    B(Scratchpad, 0x06) = id;
                } else if (role < 0xD) {
                    if (role == 0xB) {
                        B(Scratchpad, 0x07) = id;
                    }
                } else if (role == 0x13) {
                    B(Scratchpad, 0x07) = id;
                } else if (role == 0x14) {
                    B(Scratchpad, 0x06) = id;
                }
            }
        }

        p = (B(Scratchpad, 0x2CF + B(D_800FF748, 0x2E90)) == 0) ? (D_800FF748 + 0x2AF8) : (D_800FF748 + 0x55F0);
        owner = B(Scratchpad, 0x2A9);
        for (i = 0; i < 10; i++, p -= 0x3E8) {
            unsigned char id = B(p, 0x38D);
            unsigned int role_idx;

            if (id == owner || B(p, 0x00) == 0 || id == B(Scratchpad, 0x31E + (B(p, 0x399) << 1))) {
                continue;
            }
            role_idx = (unsigned int)(B(p, 0x391) - 3);
            if (role_idx >= 0xAU) {
                continue;
            }
            switch (role_idx) {
            case 0:
                if (B(Scratchpad, 0x2FF + B(p, 0x398)) < 2 && D_800CB650 != 0) {
                    if (B(Scratchpad, 0x07) == 0) {
                        B(Scratchpad, 0x07) = id;
                    }
                }
                break;
            case 1:
                if (B(Scratchpad, 0x2FF + B(p, 0x398)) < 2 && D_800CB650 != 0) {
                    if (B(Scratchpad, 0x06) == 0) {
                        B(Scratchpad, 0x06) = id;
                    }
                }
                break;
            case 3:
            case 7:
            case 8:
                if (B(Scratchpad, 0x07) == 0) {
                    B(Scratchpad, 0x07) = id;
                }
                break;
            case 4:
            case 6:
            case 9:
                if (B(Scratchpad, 0x06) == 0) {
                    B(Scratchpad, 0x06) = id;
                }
                break;
            case 2:
            case 5:
            default:
                break;
            }
        }

        p = (B(Scratchpad, 0x2CF + B(D_800FF748, 0x2E90)) == 0) ? (D_800FF748 + 0x2AF8) : (D_800FF748 + 0x55F0);
        owner = B(Scratchpad, 0x2A9);
        for (i = 0; i < 10; i++, p -= 0x3E8) {
            unsigned char id = B(p, 0x38D);
            unsigned int role_idx;

            if (id == owner || B(p, 0x00) == 0 || id == B(Scratchpad, 0x31E + (B(p, 0x399) << 1))) {
                continue;
            }
            role_idx = (unsigned int)(B(p, 0x391) - 3);
            if (role_idx >= 0xEU) {
                continue;
            }
            switch (role_idx) {
            case 5:
            case 10:
            case 13:
                if (B(p, 0x399) != 0) {
                    if (S(p, 0x06) < 0) {
                        if (B(Scratchpad, 0x06) == 0) {
                            B(Scratchpad, 0x06) = id;
                        }
                    } else {
                        if (B(Scratchpad, 0x07) == 0) {
                            B(Scratchpad, 0x07) = id;
                        }
                    }
                } else {
                    if (S(p, 0x06) > 0) {
                        if (B(Scratchpad, 0x06) == 0) {
                            B(Scratchpad, 0x06) = id;
                        }
                    } else {
                        if (B(Scratchpad, 0x07) == 0) {
                            B(Scratchpad, 0x07) = id;
                        }
                    }
                }
                break;
            case 1:
            case 4:
            case 6:
            case 9:
            case 12:
                if (B(Scratchpad, 0x06) == 0) {
                    B(Scratchpad, 0x06) = id;
                }
                break;
            case 0:
            case 3:
            case 7:
            case 8:
            case 11:
                if (B(Scratchpad, 0x07) == 0) {
                    B(Scratchpad, 0x07) = id;
                }
                break;
            case 2:
            default:
                break;
            }
        }
    }

    run_mode &= 0xFF;
    if (run_mode == 2) {
        unsigned char id = B(Scratchpad, 0x07);
        unsigned char *chosen = D_800FF748 + (id % 24) * 1000;
        B(Scratchpad, 0x06) = 0;
        B(chosen, 0x3E2) = 8;
        B(chosen, 0x3E3) = 0;
    } else if (run_mode == 3) {
        unsigned char id = B(Scratchpad, 0x06);
        unsigned char *chosen = D_800FF748 + (id % 24) * 1000;
        B(Scratchpad, 0x07) = 0;
        B(chosen, 0x3E2) = 9;
        B(chosen, 0x3E3) = 0;
    }
}
