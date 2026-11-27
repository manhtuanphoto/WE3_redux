/*
 * func_80120A58 (selform overlay)
 *
 * Purpose:
 *   Redraws all visible player rows in the formation/lineup list for `side`
 *   across the scroll window [scroll_start .. scroll_end] (from D_80123020[side]).
 *   Updates each row's two-digit/single-digit jersey number sprites, form/
 *   condition arrow, card/suspension/injury status icon, starter vs reserve
 *   background bar, position badge (from D_800CA30C for starters or
 *   D_800EF789 for reserves), and unavailability dimming (palette 0x6A).
 *
 * Arguments:
 *   side_arg (unsigned int): side index in low byte (0 = home / P1, 1 = away / P2)
 *
 * Return value:
 *   None (void)
 *
 * Global fields read / written:
 *   D_800CA30C (u8[])         - formation slot position table (11 bytes per formation)
 *   D_800EF788 (u8[])         - player record table (4 bytes per player, 22 players = 88 bytes per side):
 *     +0x00 (u8)              - jersey/shirt number
 *     +0x01 (u8)              - player natural position ID
 *     +0x02 (u8)              - player name/text tile or attribute index
 *     +0x03 (D_800EF78B, u8)  - status/availability flags (bit 4 = 0x10 unavailable)
 *   D_800F0FEC (u8 *)         - temporary pointer to current player's 4-byte D_800EF788 record
 *   D_800FF748 (u8[])         - match/menu state base:
 *     +0x88A5 + side*22 + p   - player condition/availability byte (>= 2 = dimmed)
 *     +0x8E9C + side*22 + i   - squad slot array (22 player indices per side)
 *     +0x93A8 + team*132 + p*6- cup/league yellow-card/status byte (low nibble for D_8012235C lookup)
 *     +0x93A9 + team*132 + p*6- cup/league suspension flag (1 = suspended/red-carded)
 *   D_8012235C (u8[15 * 2])   - status code lookup table (stride 2: +0 key, +1 D_8012235D icon ID)
 *   D_8012235D (u8[15 * 2])   - icon ID paired with D_8012235C
 *   D_80123020 (u8[2][0x12])  - per-side cursor/scroll struct (+0xC scroll_start, +0xE scroll_end)
 *   D_80123044 (u8 *)         - pointer to active side's D_80123020 entry
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x2DD + side (u8)        - team ID per side
 *   +0x2E1 (u8)               - competition/game mode (1..2 enables cup/league suspension/card icons)
 *   +0x2FD + side (u8)        - formation index per side (used to index D_800CA30C)
 */

#define B(p, o) (*(unsigned char *)((p) + (o)))
#define H(p, o) (*(unsigned short *)((p) + (o)))
#define S(p, o) (*(signed short *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800CA30C[];
extern unsigned char D_800EF788[];
extern unsigned char D_800EF78B[];
extern unsigned char *D_800F0FEC;
extern unsigned char D_800FF748[];
extern unsigned char D_8012235C[];
extern unsigned char D_8012235D[];
extern unsigned char D_80123020[];
extern unsigned char *D_80123044;

extern unsigned char *func_8001D004(unsigned int id);
extern void func_8006FAF4(unsigned int slot, unsigned int sub_slot, unsigned int val);
extern void func_8006FB30(unsigned int slot, unsigned int sub_slot, unsigned int val1, unsigned int val2);
extern void func_8006FB90(unsigned int slot, unsigned int sub_slot, unsigned int val);
extern void func_8006FBCC(unsigned int slot, unsigned int sub_slot, unsigned int val);

void func_80120A58(unsigned int side_arg)
{
    unsigned int side = side_arg & 0xFFU;
    unsigned char *cur = D_80123020 + side * 0x12U;
    unsigned int i;
    unsigned char *squad = D_800FF748 + 0x8E9CU + side * 22U;
    unsigned int team_base = side * 88U;

    D_80123044 = cur;
    i = H(cur, 0xC);
    if (H(cur, 0xE) < i) {
        return;
    }

    do {
        unsigned int player_idx = squad[i];
        unsigned char *rec = D_800EF788 + team_base + player_idx * 4U;
        unsigned char *num_spr;
        unsigned int num;
        unsigned int tens;
        unsigned int ones;
        unsigned int row;
        unsigned char *entry;
        unsigned int comp_mode;
        unsigned int pos;

        D_800F0FEC = rec;
        num_spr = func_8001D004(side + 0x73U);
        num = D_800F0FEC[0];
        tens = (num / 10U) & 0xFFU;
        ones = (num % 10U) & 0xFFU;
        row = i - H(D_80123044, 0xC);
        entry = num_spr + row * 24U;

        if (tens != 0) {
            entry[0x0A] = 0x1D;
            entry[0x04] = (unsigned char)((tens << 3) - 0x80);
            entry[0x10] = (unsigned char)((ones << 3) - 0x80);
            S(entry, 0x12) = (signed short)(side * 180U - 0x5EU);
        } else {
            entry[0x0A] = 0x5B;
            entry[0x10] = (unsigned char)((ones << 3) - 0x80);
            S(entry, 0x12) = (signed short)(side * 180U - 0x62U);
        }

        /* Player name / attribute tile */
        row = i - H(D_80123044, 0xC);
        func_8006FBCC(side + 0x16U, row, (unsigned int)(D_800F0FEC[2] << 4) + 0x30U);
        row = i - H(D_80123044, 0xC);
        func_8006FAF4(side + 0x16U, row, (unsigned int)D_800F0FEC[2] + 0x6BU);

        /* Condition / card / suspension icon */
        comp_mode = B(Scratchpad, 0x2E1);
        if ((unsigned int)(comp_mode - 1U) < 2U) {
            unsigned int team_id = B(Scratchpad, 0x2DD + side);
            unsigned int cup_off = team_id * 132U + player_idx * 6U;
            if (D_800FF748[0x93A9U + cup_off] == 1) {
                row = i - H(D_80123044, 0xC);
                func_8006FB30(side + 0x14U, row, 8, 0x80);
            } else {
                unsigned int key = D_800FF748[0x93A8U + cup_off] |
                                   (D_800FF748[0x88A5U + side * 22U + player_idx] << 4);
                if (key == 0) {
                    row = i - H(D_80123044, 0xC);
                    func_8006FB30(side + 0x14U, row, 0x50, 0x90);
                } else {
                    unsigned int j;
                    for (j = 0; j < 0xFU; j++) {
                        if (D_8012235C[j * 2U] == key) {
                            row = i - H(D_80123044, 0xC);
                            func_8006FB30(side + 0x14U, row, D_8012235D[j * 2U], 0x80);
                        }
                    }
                }
            }
        } else {
            unsigned int key = D_800FF748[0x88A5U + side * 22U + player_idx] << 4;
            if (key == 0) {
                row = i - H(D_80123044, 0xC);
                func_8006FB30(side + 0x14U, row, 0x50, 0x90);
            } else {
                unsigned int j;
                for (j = 0; j < 0xFU; j++) {
                    if (D_8012235C[j * 2U] == key) {
                        row = i - H(D_80123044, 0xC);
                        func_8006FB30(side + 0x14U, row, D_8012235D[j * 2U], 0x80);
                    }
                }
            }
        }

        /* Player slot indicator (starter 0..10 vs reserve 11..21) */
        player_idx = squad[i];
        row = i - H(D_80123044, 0xC);
        if (player_idx < 0xBU) {
            func_8006FB30(side + 0xEU, row, side << 7, player_idx << 4);
        } else {
            func_8006FB30(side + 0xEU, row, (side << 7) + 0x40U, (player_idx - 0xBU) << 4);
        }

        /* Row background palette & position badge */
        row = i - H(D_80123044, 0xC);
        if (i >= 0xBU) {
            func_8006FAF4(side + 0xEU, row, 0x69);
            pos = D_800F0FEC[1];
        } else {
            func_8006FAF4(side + 0xEU, row, 8);
            pos = D_800CA30C[B(Scratchpad, 0x2FD + side) * 11U + i];
        }
        row = i - H(D_80123044, 0xC);
        func_8006FB90(side + 0xCU, row, pos << 4);

        /* Dim/mark unavailable players (palette 0x6A) */
        player_idx = squad[i];
        if (D_800FF748[0x88A5U + side * 22U + player_idx] >= 2) {
            row = i - H(D_80123044, 0xC);
            func_8006FAF4(side + 0xEU, row, 0x6A);
        }

        if ((unsigned int)(B(Scratchpad, 0x2E1) - 1U) < 2U) {
            unsigned int team_id = B(Scratchpad, 0x2DD + side);
            if (D_800FF748[0x93A9U + team_id * 132U + squad[i] * 6U] == 1) {
                row = i - H(D_80123044, 0xC);
                func_8006FAF4(side + 0xEU, row, 0x6A);
            }
        }

        if ((D_800EF78B[team_base + squad[i] * 4U] & 0x10U) != 0) {
            row = i - H(D_80123044, 0xC);
            func_8006FAF4(side + 0xEU, row, 0x6A);
        }

        i = (i + 1U) & 0xFFFFU;
    } while (i <= H(D_80123044, 0xE));
}
