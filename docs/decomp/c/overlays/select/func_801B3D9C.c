/*
 * func_801B3D9C (overlay: select)
 *
 * Purpose:
 *   Populates group standings UI widgets (0x30A1, 0x30AB, 0x30A4..0x30A9) and
 *   formats the 45-byte text/control-code entries in D_801D94E8 for a given
 *   group index (`arg0`, reduced by 0x41 if >= 0x41). Clears D_801D94E8 (65
 *   entries x 45 bytes), configures group header/rank flags, team stats
 *   (W/D/L/Pts and GF/GA via func_801B0368), the 4x4 head-to-head results grid
 *   (D_800FF748 + 0xA9D0), and builds formatted team lines via func_801A9168.
 *
 * Arguments:
 *   group_idx (unsigned int): group index (if (group_idx & 0xFF) >= 0x41,
 *                             0x41 is subtracted before masking to 0xFF)
 *
 * Return value:
 *   unsigned char *: pointer to the group's 4-team standings descriptor
 *                    returned by func_801B4C84
 *
 * Globals / Structs read & written:
 *   D_801D94E8 (u8[65 * 45], W): cleared to 0, then populated with entries 0..8
 *   D_800FF748 (u8[], R):
 *     +0x8F24 + team * 36 (u8): team flags (0xC0 and 0x3F passed to func_801A9168)
 *     +0x8F25 + team * 36 (u8): team short/badge code byte
 *     +0x8F28..+0x8F2B + team * 36 (u8[4]): 4 single-byte stats (rendered at +0x03)
 *     +0x8F2C, +0x8F2E + team * 36 (u16[2]): 2 two-byte stats (func_801B0368)
 *     +0xA9D0 + (team_slot * 3 + m) * 4 (u8): match result (1=win, 2=draw, 3=loss)
 *     +0xA9D1 + (team_slot * 3 + m) * 4 (u8): opponent team slot index
 *     +0xAB80 + slot (u8): slot-to-team ID mapping
 *   D_801D878C..D_801D8790 (u8, R/W): scratch globals for group base / team /
 *                                     result glyph & value
 *
 * Scratchpad (0x1F800000) read:
 *   +0x29B (u8, R): sub-mode (0x0A shows placeholder/hidden rank markers 0x5B/0xDE)
 */

#define B(p, o) (*(unsigned char *)((p) + (o)))
#define H(p, o) (*(unsigned short *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned char D_801D878C;
extern unsigned char D_801D878D;
extern unsigned char D_801D878E;
extern unsigned char D_801D878F;
extern unsigned char D_801D8790;
extern unsigned char D_801D94E8[];

extern unsigned char *func_8001D004(int id);
extern unsigned char *func_801A9168(unsigned char *dst, int a1, unsigned int a2, unsigned int a3);
extern unsigned char *func_801B0368(unsigned char *widget, unsigned short val);
extern unsigned char *func_801B4C84(unsigned int group_idx);

unsigned char *func_801B3D9C(unsigned int group_idx)
{
    unsigned char *base = D_800FF748;
    unsigned char *standings;
    unsigned char *obj;
    unsigned int grp;
    int s0;
    int s1;
    int s3;

    if ((group_idx & 0xFFu) >= 0x41u) {
        group_idx -= 0x41u;
    }

    grp = group_idx & 0xFFu;
    D_801D878C = (unsigned char)(grp << 2);

    standings = func_801B4C84(grp);

    /* Clear 65 entries x 45 bytes (0x2D) in D_801D94E8 */
    for (s1 = 0; s1 < 0x41; s1++) {
        unsigned char *rec = D_801D94E8 + s1 * 0x2D;
        for (s0 = 0x2C; s0 >= 0; s0--) {
            rec[s0] = 0;
        }
        s0 = 0x2D;
    }

    obj = func_8001D004(0x30A1);
    grp = group_idx & 0xFFu;
    B(obj, 0x03) = (unsigned char)(grp * 24u);

    obj = func_8001D004(0x30AB);
    B(obj, 0x0A) = 0x0A;
    B(obj, 0x16) = 0xD5;
    B(obj, 0x22) = 0xD5;
    B(obj, 0x2E) = 0x0A;
    B(obj, 0x3A) = 0x0A;
    B(obj, 0x46) = 0xD5;
    B(obj, 0x52) = 0xD5;

    if (grp == 0u) {
        B(obj, 0x0A) = 0x5B;
        B(obj, 0x16) = 0x5B;
        B(obj, 0x22) = 0x5B;
    } else if (grp == 7u) {
        B(obj, 0x2E) = 0x5B;
        B(obj, 0x3A) = 0x5B;
        B(obj, 0x46) = 0x5B;
        B(obj, 0x52) = 0x5B;
    }

    /* NOTE: s0 is 0x2D after the clear loop above, so s0 >= 4 is always true. */
    if (s0 >= 4) {
        obj = func_8001D004(0x30A4);
        if (B(Scratchpad, 0x29B) == 0x0A) {
            B(obj, 0x0A) = 0x5B;
            B(obj, 0x16) = 0xDE;
        } else {
            B(obj, 0x0A) = 0x0E;
            B(obj, 0x16) = 0x5B;
            B(obj, 0x03) = (unsigned char)((standings[1] << 3) - 0x50);
        }

        obj = func_8001D004(0x30A5);
        if (B(Scratchpad, 0x29B) == 0x0A) {
            B(obj, 0x0A) = 0x5B;
            B(obj, 0x16) = 0xDE;
        } else {
            B(obj, 0x0A) = 0x0E;
            B(obj, 0x16) = 0x5B;
            B(obj, 0x03) = (unsigned char)((standings[3] << 3) - 0x50);
        }

        obj = func_8001D004(0x30A6);
        B(obj, 0x0A) = 0x0E;
        B(obj, 0x03) = (unsigned char)((standings[5] << 3) - 0x50);

        obj = func_8001D004(0x30A7);
        B(obj, 0x0A) = 0x0E;
        B(obj, 0x03) = (unsigned char)((standings[7] << 3) - 0x50);
    }

    /* Standings numeric columns for the 4 ranked teams (0x30A8) */
    obj = func_8001D004(0x30A8);
    for (s1 = 0; s1 < 4; s1++) {
        unsigned int team;

        D_801D878D = standings[s1 * 2];
        team = D_801D878D;
        B(obj, 0x03) = (unsigned char)((B(base, 0x8F28 + team * 36) << 3) - 0x50);

        obj += 0x0C;
        team = D_801D878D;
        B(obj, 0x03) = (unsigned char)((B(base, 0x8F29 + team * 36) << 3) - 0x50);

        obj += 0x0C;
        team = D_801D878D;
        B(obj, 0x03) = (unsigned char)((B(base, 0x8F2A + team * 36) << 3) - 0x50);

        obj += 0x0C;
        team = D_801D878D;
        B(obj, 0x03) = (unsigned char)((B(base, 0x8F2B + team * 36) << 3) - 0x50);

        team = D_801D878D;
        obj = func_801B0368(obj + 0x0C, H(base, 0x8F2C + team * 36));
        team = D_801D878D;
        obj = func_801B0368(obj, H(base, 0x8F2E + team * 36));
    }

    /* 4x4 group match results grid (0x30A9) */
    obj = func_8001D004(0x30A9);
    {
        unsigned int team_base = (group_idx & 0xFFu) << 2;
        D_801D878C = (unsigned char)team_base;

        for (s1 = 0; s1 < 4; s1++) {
            unsigned char *cell = obj;
            for (s0 = 0; s0 < 4; s0++) {
                B(cell, 0x0A) = 0x5B;
                if (s1 != s0) {
                    unsigned int target_opp = team_base + (unsigned int)s0;
                    int m;
                    for (m = 0; m < 3; m++) {
                        unsigned int match_off = ((D_801D878C + (unsigned int)s1) * 3u + (unsigned int)m) * 4u;
                        unsigned char res = B(base, 0xA9D0 + match_off);
                        unsigned char opp = B(base, 0xA9D1 + match_off);

                        D_801D878E = res;
                        D_801D878D = opp;
                        if (opp == target_opp) {
                            if (res == 1) {
                                D_801D8790 = 0x7D;
                                D_801D878F = 0x00;
                            } else if (res == 2) {
                                D_801D8790 = 0x7E;
                                D_801D878F = 0x10;
                            } else if (res == 3) {
                                D_801D8790 = 0x7F;
                                D_801D878F = 0x20;
                            }
                            B(cell, 0x0A) = D_801D8790;
                            B(cell, 0x03) = D_801D878F;
                        }
                    }
                }
                cell += 0x0C;
            }
            obj += 0x30;
        }
    }

    /* Format D_801D94E8 entries 0..3 (ranked group order) */
    s0 = 0;
    s3 = 0;
    for (s1 = 0; s1 < 4; s1++) {
        unsigned int team;
        unsigned char flags;
        unsigned char *p;

        D_801D878D = standings[s1 * 2];
        team = B(base, 0xAB80 + D_801D878D);
        flags = B(base, 0x8F24 + team * 36);
        p = func_801A9168(D_801D94E8 + s3, 4, flags & 0xC0u, (flags & 0x3Fu) + 1u);

        p[0] = 0x09;
        p[1] = 0x1C;
        p[2] = 0x1E;
        team = B(base, 0xAB80 + D_801D878D);
        p[3] = B(base, 0x8F25 + team * 36);
        p[4] = 0x20;
        p[5] = 0x1C;
        team = B(base, 0xAB80 + D_801D878D);
        p[6] = B(base, 0x8F25 + team * 36);

        s0++;
        s3 += 0x2D;
    }

    /* Format D_801D94E8 entries 4..7 (fixed group slot order) */
    {
        unsigned int team_base = (group_idx & 0xFFu) << 2;
        s3 = s0 * 0x2D;

        for (s1 = 0; s1 < 4; s1++) {
            unsigned int team;
            unsigned char flags;
            unsigned char *p;

            D_801D878D = (unsigned char)(team_base + (unsigned int)s1);
            team = B(base, 0xAB80 + D_801D878D);
            flags = B(base, 0x8F24 + team * 36);
            p = func_801A9168(D_801D94E8 + s3, 4, flags & 0xC0u, (flags & 0x3Fu) + 1u);

            p[0] = 0x09;
            p[1] = 0x1C;
            p[2] = 0x1E;
            team = B(base, 0xAB80 + D_801D878D);
            p[3] = B(base, 0x8F25 + team * 36);
            p[4] = 0x20;
            p[5] = 0x1C;
            team = B(base, 0xAB80 + D_801D878D);
            p[6] = B(base, 0x8F25 + team * 36);

            s0++;
            s3 += 0x2D;
        }

        /* Format D_801D94E8 entry 8 (summary / compact team strip) */
        {
            unsigned char *p = D_801D94E8 + s0 * 0x2D;
            for (s1 = 0; s1 < 4; s1++) {
                unsigned int team = B(base, 0xAB80 + team_base + (unsigned int)s1);
                p[0] = 0x1E;
                p[1] = B(base, 0x8F25 + team * 36);
                p[2] = 0x20;
                p[3] = 0x09;
                p[4] = (unsigned char)((s1 + 1) * 36);
                p += 5;
            }
        }
    }

    return standings;
}
