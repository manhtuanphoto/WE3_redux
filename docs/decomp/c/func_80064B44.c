/*
 * func_80064B44
 *
 * Purpose:
 *   Computes an 8-sector (45-deg each) obstacle/opponent danger map in
 *   D_8010C1D8[0x16..0x1D] around the given player actor for CPU dribbling/
 *   steering decisions, factoring in opponent proximity rings, predicted
 *   opponent velocities, opponent states, body-turn cost, and pitch boundaries.
 *
 * Arguments:
 *   actor (unsigned char *): pointer to 1000-byte player actor struct.
 *
 * Return value:
 *   void (writes 8 byte scores into D_8010C1D8 + 0x16).
 *
 * Scratchpad (0x1F800000) reads:
 *   - 0x290 (s32, R): global frame counter (gates update rate and marking filter).
 *   - 0x31E (u8[2][2], R): marked/closest opponent IDs per attack direction.
 *   - 0x328 (u8, R): CPU difficulty/update-rate level (0, 1, or >=2).
 *
 * Actor fields reads:
 *   - +0x00 (u8, R): active flag (must be nonzero).
 *   - +0x02/+0x06 (s16, R): position x/z.
 *   - +0x38D (u8, R): actor ID (passed to func_8005CE30 to get opponent base index).
 *   - +0x399 (u8, R): attack direction flag (0/1).
 *   - +0x3AA (u8, R): body heading (0..255 = 360 deg).
 *
 * Opponent actor fields reads:
 *   - +0x02/+0x06 (s16, R): opponent x/z.
 *   - +0x08/+0x10 (s32, R): opponent velocity x/z (projected 2 frames forward).
 *   - +0x38D (u8, R): opponent actor ID.
 *   - +0x396 (u8, R): opponent state (0xD halves dist_sq, 0x10 skipped).
 *
 * Magic numbers (250 raw = 1 m, 30 Hz):
 *   - Update gating: mode 0 -> every 10 frames (0.33 s); mode 1 -> every 10 frames
 *     for first 120 frames (4 s) of 240-frame (8 s) cycle, every 5 frames (0.17 s)
 *     for second 4 s; mode >= 2 -> every frame.
 *   - 0xB4 / 360: first 180 frames (6 s) of 360-frame (12 s) cycle restrict checks
 *     to the two marked opponents in scratch 0x31E/0x31F.
 *   - Distance rings (dist_sq):
 *     - 0x40000 (512^2) = 2.048 m; 0x100000 (1024^2) = 4.096 m;
 *     - 0x240000 (1536^2) = 6.144 m; 0x1000000 (4096^2) = 16.384 m.
 *     - Scaled by 2/3 when D_8010C1D8[0] != 0.
 *   - Pitch bounds:
 *     - X >= 0x2A21 (43.14 m) / 0x2E21 (47.24 m), X < -0x2B20 (-44.16 m) / -0x2E20 (-47.23 m).
 *     - Z >= 0x1B01 (27.65 m) / 0x1C01 (28.68 m), Z < -0x1B00 (-27.65 m) / -0x1C00 (-28.67 m).
 *     - Score 0x10 (16) marks a sector hard-blocked at the outer touchline/goal line.
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))

extern unsigned char D_8010C1D8[];
extern unsigned char D_800FF748[];
extern unsigned char D_800C851C[];

extern int func_8005CE30(int actor_id);
extern int func_8005B36C(int x1, int z1, int x2, int z2);

void func_80064B44(unsigned char *actor)
{
    unsigned char *scratch = (unsigned char *)0x1F800000;
    unsigned char diff_lvl;
    int frame;
    int only_marked;
    int i;
    int opp_start;
    unsigned char *opp;

    diff_lvl = B(scratch, 0x328);
    if (diff_lvl == 1) {
        frame = SW(scratch, 0x290);
        if (frame % 5 != 0) {
            return;
        }
        if (frame % 240 < 0x78 && frame % 10 != 0) {
            return;
        }
    } else if (diff_lvl < 2) {
        frame = SW(scratch, 0x290);
        if (frame % 10 != 0) {
            return;
        }
    }

    frame = SW(scratch, 0x290);
    only_marked = (frame % 360) < 0xB4;

    for (i = 0; i < 8; i++) {
        D_8010C1D8[0x16 + i] = 0;
    }

    opp_start = func_8005CE30(B(actor, 0x38D)) & 0xFF;
    opp = D_800FF748 + (opp_start % 24) * 1000;

    for (i = 0; i < 11; i++, opp += 0x3E8) {
        int dx, dz;
        unsigned int dist_sq;
        unsigned char state;
        int tx, tz;
        int opp_angle;
        int ring;

        if (only_marked) {
            unsigned char opp_id = B(opp, 0x38D);
            int idx = (B(actor, 0x399) ^ 1) * 2;
            if (opp_id != B(scratch, 0x31E + idx) && opp_id != B(scratch, 0x31F + idx)) {
                continue;
            }
        }

        dx = S(actor, 2) - S(opp, 2);
        dz = S(actor, 6) - S(opp, 6);
        dist_sq = (unsigned int)(dx * dx + dz * dz);

        if (B(actor, 0) == 0 || dist_sq > 0xFFFFFFU) {
            continue;
        }

        state = B(opp, 0x396);
        if (state == 0xD) {
            tx = S(opp, 2);
            tz = S(opp, 6);
            dist_sq >>= 1;
        } else if (state == 0x10) {
            continue;
        } else {
            tx = S(opp, 2) + SW(opp, 8) * 2;
            tz = S(opp, 6) + SW(opp, 0x10) * 2;
        }

        opp_angle = func_8005B36C(S(actor, 2), S(actor, 6), (signed short)tx, (signed short)tz);

        for (ring = 0; ring < 4; ring++) {
            unsigned int thresh;
            int mode;
            unsigned char strict;
            int dir_idx;

            if (ring == 0) {
                thresh = 0x40000;
                mode = 3;
            } else if (ring == 1) {
                thresh = 0x100000;
                mode = 2;
            } else if (ring == 2) {
                thresh = 0x240000;
                mode = 1;
            } else {
                thresh = 0x1000000;
                mode = 0;
            }

            strict = D_8010C1D8[0];
            if (strict != 0) {
                thresh = (thresh * 2) / 3;
            }

            if (dist_sq >= thresh) {
                continue;
            }
            if (ring == 0 && strict != 0) {
                break;
            }

            for (dir_idx = 0; dir_idx < 8; dir_idx++) {
                int sector_angle = dir_idx << 5;
                int diff = sector_angle - opp_angle;
                int abs_diff = ((diff & 0xFF) < 0x81) ? diff : (opp_angle - sector_angle);
                unsigned char ad = (unsigned char)abs_diff;

                if (mode == 0) {
                    if (ad < 0x20 && D_8010C1D8[0x16 + dir_idx] == 0) {
                        D_8010C1D8[0x16 + dir_idx] = 1;
                    }
                } else if (mode == 1) {
                    if (ad < 0x28) {
                        if (D_8010C1D8[0x16 + dir_idx] == 0) {
                            D_8010C1D8[0x16 + dir_idx] = 1;
                        }
                        if (D_8010C1D8[0x16 + dir_idx] < 3) {
                            D_8010C1D8[0x16 + dir_idx] += 1;
                        }
                    }
                } else if (mode == 2) {
                    if (ad < 0x30 && D_8010C1D8[0x16 + dir_idx] == 0) {
                        D_8010C1D8[0x16 + dir_idx] = 1;
                    }
                    if (ad < 0x38 && D_8010C1D8[0x16 + dir_idx] < 4) {
                        D_8010C1D8[0x16 + dir_idx] += 1;
                    }
                } else if (mode == 3) {
                    if (ad < 0x48 && D_8010C1D8[0x16 + dir_idx] == 0) {
                        D_8010C1D8[0x16 + dir_idx] = 1;
                    }
                    if (ad < 0x40 && D_8010C1D8[0x16 + dir_idx] < 4) {
                        D_8010C1D8[0x16 + dir_idx] += 1;
                    }
                    switch (B(opp, 0x396)) {
                    case 3:
                    case 0xC:
                    case 0xD:
                    case 0xF:
                    case 0x16:
                    case 0x17:
                        if (ad < 0x60) {
                            D_8010C1D8[0x16 + dir_idx] += 1;
                        }
                        break;
                    default:
                        break;
                    }
                }
            }
        }
    }

    for (i = 0; i < 8; i++) {
        int sector_angle = D_800C851C[i];
        int body_angle = B(actor, 0x3AA);
        int diff = sector_angle - body_angle;
        int abs_diff = ((diff & 0xFF) < 0x81) ? diff : (body_angle - sector_angle);
        if ((abs_diff & 0xFF) >= 0x60) {
            D_8010C1D8[0x16 + i] += 1;
        }
    }

    if (S(actor, 2) >= 0x2A21) {
        if (D_8010C1D8[0x16] == 0) {
            D_8010C1D8[0x16] = 1;
        }
        /* UNCLEAR: both checks increment sector 0 (0x16) rather than sectors 1/7 (0x17/0x1D) */
        if (D_8010C1D8[0x17] == 0) {
            D_8010C1D8[0x16] += 1;
        }
        if (D_8010C1D8[0x1D] == 0) {
            D_8010C1D8[0x16] += 1;
        }
    }
    if (S(actor, 2) >= 0x2E21) {
        D_8010C1D8[0x16] = 0x10;
        D_8010C1D8[0x17] = 0x10;
        D_8010C1D8[0x1D] = 0x10;
    }
    if (S(actor, 2) < -0x2B20) {
        for (i = 3; i < 6; i++) {
            if (D_8010C1D8[0x16 + i] == 0) {
                D_8010C1D8[0x16 + i] = 1;
            }
        }
    }
    if (S(actor, 2) < -0x2E20) {
        D_8010C1D8[0x19] = 0x10;
        D_8010C1D8[0x1A] = 0x10;
        D_8010C1D8[0x1B] = 0x10;
    }

    if (S(actor, 6) >= 0x1C01) {
        D_8010C1D8[0x17] = 0x10;
        D_8010C1D8[0x18] = 0x10;
        D_8010C1D8[0x19] = 0x10;
    }
    if (S(actor, 6) >= 0x1B01) {
        for (i = 1; i < 4; i++) {
            if (D_8010C1D8[0x16 + i] == 0) {
                D_8010C1D8[0x16 + i] = 1;
            }
        }
    }
    if (S(actor, 6) < -0x1C00) {
        D_8010C1D8[0x1C] = 0x10;
        D_8010C1D8[0x1B] = 0x10;
        D_8010C1D8[0x1D] = 0x10;
    }
    if (S(actor, 6) < -0x1B00) {
        for (i = 5; i < 8; i++) {
            if (D_8010C1D8[0x16 + i] == 0) {
                D_8010C1D8[0x16 + i] = 1;
            }
        }
    }
}
