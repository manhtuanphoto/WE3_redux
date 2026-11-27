/*
 * func_80072A2C
 *
 * Purpose:
 *   Match/phase lineup, substitution, and squad-slot synchronization handler.
 *   Clears player active flags on mode/phase resets, processes red-card /
 *   unavailable player flags (D_800EF78B) to record substitutions into
 *   D_800E7660 / D_8010A5D8 and update D_800E6CD0, dispatches match-phase
 *   handlers (func_8001F53C, func_800772F8, func_80079D5C, func_8007A774,
 *   func_8005ADE4, func_8007B41C, func_8004A2D0), assigns player roles (+0x391)
 *   from formation table D_800C8118, and copies the 44-byte squad array to
 *   D_800E7620 when appropriate.
 *
 * Arguments:
 *   None.
 *
 * Return value:
 *   void
 *
 * Scratchpad / Actor fields read or written:
 *   Scratch reads:  0x29A (match phase), 0x29B (phase mode), 0x29C, 0x2A0,
 *                   0x2A1, 0x2C0, 0x2D0, 0x2DF, 0x2E0, 0x2E1 (mode),
 *                   0x2FD..0x2FE (team formation index), 0x333, 0x334..0x335
 *   Scratch writes: 0x294 (s16 timer reset), 0x29B (phase mode), 0x333,
 *                   0x334..0x335 (per-team sub count)
 *   Actor reads:    +0x392 (team slot 0..10)
 *   Actor writes:   +0x00 (active/enabled flag cleared to 0), +0x391 (role ID)
 *
 * Magic numbers / units:
 *   22 players = 0x16 (11 starters = 0x0B + 11 bench); actor stride = 0x3E8 (1000).
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define SCRATCH ((unsigned char*)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned char D_800EF78B[];
extern unsigned char D_8010C538[];
extern unsigned char D_800E7660[];
extern unsigned char D_8010A5D8[];
extern unsigned char D_800E6CD0[];
extern unsigned char D_800C8118[];
extern unsigned char D_800E7620[];

void func_800729CC(void);
void func_800772F8(void);
void func_80079D5C(void);
void func_8007B41C(void);
void func_8001F53C(void);
void func_8005ADE4(void);
void func_8007A774(void);
void func_8004A2D0(void);

void func_80072A2C(void)
{
    int team;
    int i;
    int j;
    unsigned char found_pending_sub;
    unsigned char *actor_ptr;

    if (B(SCRATCH, 0x2E1) == 5) {
        func_800729CC();
        if (B(SCRATCH, 0x29A) == 0xD) {
            actor_ptr = D_800FF748 + 0x3E8;
            for (i = 21; i >= 0; i--) {
                B(actor_ptr, 0) = 0;
                actor_ptr += 0x3E8;
            }
        }

        if (B(SCRATCH, 0x29A) == 2) {
            B(D_8010C538, 0) = 1;
            if (B(SCRATCH, 0x29B) < 2) {
                B(SCRATCH, 0x29B) = 0;
                func_800772F8();
            } else {
                B(SCRATCH, 0x29B) = 2;
                func_80079D5C();
            }
        } else if (B(SCRATCH, 0x29A) == 4) {
            if (B(SCRATCH, 0x29B) != 0) {
                B(SCRATCH, 0x29B) = 1;
                func_8007B41C();
            }
        }
        return;
    }

    if (B(SCRATCH, 0x2A1) == 0) {
        func_800729CC();
        actor_ptr = D_800FF748 + 0x3E8;
        for (i = 21; i >= 0; i--) {
            B(actor_ptr, 0) = 0;
            actor_ptr += 0x3E8;
        }
    }

    found_pending_sub = 0;
    for (team = 0; team < 2; team++) {
        unsigned char *squad = D_800FF748 + 0x8E9C + team * 22;
        for (i = 0; i < 22; i++) {
            unsigned char id = B(squad, i);
            unsigned char flags = B(D_800EF78B, team * 0x58 + id * 4);
            if ((flags & 0x01) != 0 && (flags & 0x12) == 0) {
                found_pending_sub = 1;
                break;
            }
        }
    }

    if (B(SCRATCH, 0x333) != 0 && found_pending_sub != 0) {
        B(D_8010C538, 0) = 0;
        if (B(SCRATCH, 0x2C0) == 1) {
            B(SCRATCH, 0x333) = 0;
            for (team = 0; team < 2; team++) {
                B(SCRATCH, 0x334 + team) = 0;
                for (i = 0; i < 3; i++) {
                    B(D_8010A5D8, team * 3 + i) = 0;
                    B(D_800E7660, team * 3 + i) = 0;
                }
            }
        }
    }

    for (team = 0; team < 2; team++) {
        unsigned char *squad = D_800FF748 + 0x8E9C + team * 22;
        unsigned char *out_lineup = D_800E6CD0 + team * 22;
        unsigned char sub_in_idx = B(SCRATCH, 0x334 + team);
        unsigned char *sub_out_ptr = D_800E7660 + team * 3 + sub_in_idx;

        for (i = 0; i < 22; i++) {
            unsigned char id = B(squad, i);
            unsigned char flags = B(D_800EF78B, team * 0x58 + id * 4);
            if ((flags & 0x01) != 0) {
                if ((flags & 0x12) == 0) {
                    if (i < 11) {
                        B(sub_out_ptr, 0) = id;
                        sub_out_ptr++;
                        B(SCRATCH, 0x334 + team)++;
                    } else {
                        B(D_8010A5D8, team * 3 + sub_in_idx) = id;
                        sub_in_idx++;
                    }
                }
                if (i < 11) {
                    B(D_800EF78B, team * 0x58 + id * 4) |= 0x02;
                } else {
                    B(D_800EF78B, team * 0x58 + id * 4) |= 0x10;
                }
            }
            B(out_lineup, i) = B(squad, i);
        }
    }

    for (team = 0; team < 2; team++) {
        int sub_count = B(SCRATCH, 0x334 + team);
        unsigned char *squad = D_800FF748 + 0x8E9C + team * 22;
        unsigned char *out_lineup = D_800E6CD0 + team * 22;

        for (i = 0; i < sub_count; i++) {
            unsigned char sub_in_id = B(D_8010A5D8, team * 3 + i);
            unsigned char sub_out_id = B(D_800E7660, team * 3 + i);
            for (j = 0; j < 11; j++) {
                if (sub_in_id == B(squad, j)) {
                    B(out_lineup, j) = sub_out_id;
                }
            }
        }
    }

    if (B(SCRATCH, 0x29C) == 5) {
        switch (B(SCRATCH, 0x29A)) {
        case 0:
            func_8001F53C();
            H(SCRATCH, 0x294) = 0;
            break;
        case 2:
            B(D_8010C538, 0) = 1;
            if (B(SCRATCH, 0x29B) < 2) {
                B(SCRATCH, 0x29B) = 0;
                func_800772F8();
            } else {
                B(SCRATCH, 0x29B) = 2;
                func_80079D5C();
            }
            func_800729CC();
            break;
        case 3:
            func_800729CC();
            H(SCRATCH, 0x294) = 0;
            if (B(SCRATCH, 0x29B) != 0) {
                B(SCRATCH, 0x29B) = 1;
                func_8007A774();
                func_8004A2D0();
            }
            break;
        case 4:
            if (B(SCRATCH, 0x29B) != 0) {
                B(SCRATCH, 0x29B) = 1;
                func_8005ADE4();
                func_8007B41C();
            }
            func_800729CC();
            break;
        case 9:
            H(SCRATCH, 0x294) = 0;
            func_800729CC();
            break;
        case 1:
        case 5:
        case 6:
        case 7:
        case 8:
        default: {
            unsigned char *role_ptr = D_800FF748 + 0x779; /* actor[1] + 0x391 */
            for (i = 0; i < 22; i++) {
                unsigned char slot = B(role_ptr, 1); /* +0x392 */
                unsigned int team_idx = (B(SCRATCH, 0x2E0) ^ (i / 11)) & 0xFF;
                unsigned char *lineup = D_800E6CD0 + team_idx * 22;
                for (j = 0; j < 11; j++) {
                    if (slot == B(lineup, j)) {
                        unsigned char form = B(SCRATCH, 0x2FD + team_idx);
                        B(role_ptr, 0) = B(D_800C8118, form * 11 + j);
                    }
                }
                role_ptr += 0x3E8;
            }
            break;
        }
        }
    }

    if (B(SCRATCH, 0x2A0) == 0 ||
        B(SCRATCH, 0x2E1) == 5 ||
        (B(SCRATCH, 0x29A) == 0 && B(SCRATCH, 0x2DF) == 0 && (W(SCRATCH, 0x2D0) & 0xFFFF00) == 0)) {
        for (team = 0; team < 2; team++) {
            unsigned char *src = D_800FF748 + 0x8E9C + team * 22;
            unsigned char *dst = D_800E7620 + team * 22;
            for (i = 0; i < 22; i++) {
                B(dst, i) = B(src, i);
            }
        }
    }
}
