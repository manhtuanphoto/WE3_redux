/*
 * func_8011ECE4 (selform overlay)
 *
 * Purpose:
 *   Per-frame input handler and UI state updater for a side's formation /
 *   lineup selection screen (SELFORM). Polls controller input via func_8006EC80,
 *   handles D-pad list navigation/scrolling, formation cycling (L1/R1),
 *   tactical/lineup sub-panel toggling (Select/Start/L2/R2), display mode
 *   toggles (Square/Triangle), player swapping/validation (Circle/Cross),
 *   swap-cursor blinking, arrow animations, and sub-panel slide transitions.
 *
 * Arguments:
 *   side_arg (unsigned int): side index in low byte (0 = home / P1, 1 = away / P2)
 *
 * Return value:
 *   int:
 *     0 = normal frame update / handled in-screen
 *     1 = Cross pressed with no active swap (exit/back)
 *     2 = Start pressed (confirm/exit formation menu)
 *
 * Global fields read / written:
 *   D_800CA2F8 (u8[4])        - arrow/page animation table (when +0x06 == 0)
 *   D_800CA2FC (u8[4])        - arrow/page animation table (when +0x06 != 0)
 *   D_800CA30C (u8[])         - formation slot position table (11 bytes per formation)
 *   D_800EF789 (u8[])         - player record byte +1 (side * 88 + player * 4; 0 vs non-0 GK/role check)
 *   D_800EF78B (u8[])         - player record byte +3 (side * 88 + player * 4; availability/ban flags, bit 4 = 0x10)
 *   D_800FF748 (u8[])         - match/menu state base:
 *     +0x5DC0 (D_80105508)    - 40-byte UI element table (+0x04 u8 active, +0x12 s16 X offset)
 *     +0x88A5 + side*22 + p   - player condition/availability byte (>= 2 = unavailable for XI)
 *     +0x8E9C + side*22 + i   - squad slot array (22 player indices per side: 0..10 XI, 11..21 subs)
 *     +0x93A9 + team*132 + p*6- cup/league player status byte (1 = suspended/unavailable)
 *     +0xABDC + side (u8)     - Square-button display mode (0..2)
 *     +0xAC2C + side*2 (s16)  - sub-panel slide X coordinate
 *   D_8010A5A4 (u16)          - polled button mask from func_8006EC80
 *   D_80122340..43 (u8[])     - 4-byte-stride UI sprite config table for Square-button mode (3 entries)
 *   D_8012234C (u8[3])        - animation frames passed to func_8006F528 (slot side + 0x10)
 *   D_80122350 (u8[2])        - animation frames passed to func_8006F528 (slot other_side + 0x1C)
 *   D_80122354 (u8[2])        - blink frame values for slot 7 (D_80123048 indicator)
 *   D_80122358 (u8[2])        - sub-panel slide animation step counter (counts down from 8)
 *   D_80123011 (u8)           - blink toggle (0/1) for D_80123048 indicator
 *   D_80123020 (u8[2][0x12])  - per-side cursor/scroll struct (stride 0x12):
 *     +0x00 (u16) cursor      - selected squad slot (0..21)
 *     +0x02 (u16) swap_active - 1 while selecting second player to swap, else 0
 *     +0x04 (u16) blink       - swap source blink toggle (0/1)
 *     +0x06 (u16) page_anim   - scroll arrow animation phase (0/1)
 *     +0x0A (u16) swap_src    - first selected slot for swap
 *     +0x0C (u16) scroll_start- top visible slot index (0..11)
 *     +0x0E (u16) scroll_end  - bottom visible slot index (10..21)
 *   D_80123044 (u8 *)         - pointer to active side's D_80123020 entry
 *   D_80123048 (u8)           - special slot index + 1 (1-based; e.g. captain/kicker/GK? UNCLEAR)
 *   D_80123049 (u8)           - side owning D_80123048
 *   D_8012304A (u8[2])        - sub-panel open flag per side (0 = player list, 1 = sub-panel)
 *
 * Scratchpad (0x1F800000) fields read / written:
 *   +0x290 (s32)              - frame counter (blink updated when % 5 == 0)
 *   +0x29A (u8)               - match/in-game flag
 *   +0x2A0 (u8)               - substitution rules enabled flag (== 1)
 *   +0x2AD (u8)               - panel/cursor index (checked / 12 == other_side)
 *   +0x2D0 (u32)              - match state flags (masked with 0x00FFFF00)
 *   +0x2DD + side (u8)        - team ID per side
 *   +0x2DF (u8)               - match/mode flag
 *   +0x2E0 (u8)               - side flip / swap XOR mask (other_side = side ^ Scratchpad[0x2E0])
 *   +0x2E1 (u8)               - competition/game mode (1..2 = cup/league checks, 5 = exhibition/free)
 *   +0x2FD + side (u8)        - formation index per side (0..19)
 *   +0x301 + side (u8)        - tactical preset / sub-panel option 0 per side (0..7)
 *   +0x303 + side (u8)        - tactical preset / sub-panel option 1 per side (0..7)
 *   +0x307 + side (u8)        - Triangle-button toggle flag per side (0/1)
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800CA2F8[];
extern unsigned char D_800CA2FC[];
extern unsigned char D_800CA30C[];
extern unsigned char D_800EF789[];
extern unsigned char D_800EF78B[];
extern unsigned char D_800FF748[];
extern unsigned short D_8010A5A4;
extern unsigned char D_80122340[];
extern unsigned char D_80122341[];
extern unsigned char D_80122342[];
extern unsigned char D_80122343[];
extern unsigned char D_8012234C[];
extern unsigned char D_80122350[];
extern unsigned char D_80122354[];
extern unsigned char D_80122358[];
extern unsigned char D_80123011;
extern unsigned char D_80123020[];
extern unsigned char *D_80123044;
extern unsigned char D_80123048;
extern unsigned char D_80123049;
extern unsigned char D_8012304A[];

extern unsigned char *func_8001D004(unsigned int id);
extern unsigned short func_8006EC80(unsigned int mode);
extern unsigned int func_8006F528(unsigned char slot_idx, const unsigned char *options, unsigned char count, unsigned char bit_limit);
extern unsigned int func_8006F624(unsigned int slot_idx, const unsigned char *options, unsigned int count);
extern unsigned int func_8006F714(unsigned int slot_idx, const unsigned char *options, unsigned int count);
extern void func_8006FA84(unsigned int slot, unsigned int param);
extern void func_8006FAF4(unsigned int slot, unsigned int sub_slot, unsigned int val);
extern void func_8006FB90(unsigned int slot, unsigned int sub_slot, unsigned int val);
extern void func_800AB620(signed int cmd);
extern void func_8012051C(unsigned int side, unsigned int which, unsigned int val);
extern void func_8012059C(unsigned int side);
extern void func_80120698(unsigned int side);
extern void func_80120974(unsigned int side);
extern void func_80120A58(unsigned int side);
extern void func_801211E8(unsigned int other_side, unsigned int side);
extern void func_80121670(unsigned int side);
extern void func_8012199C(unsigned int side, unsigned char cursor);
extern unsigned int func_80121A2C(unsigned int side);

int func_8011ECE4(unsigned int side_arg)
{
    unsigned int side = side_arg & 0xFFU;
    unsigned int other_side = side_arg ^ B(Scratchpad, 0x2E0);
    unsigned short btn;
    unsigned char *cur;
    unsigned char *squad;
    unsigned int team_base;
    unsigned int i;

    cur = D_80123020 + side * 0x12U;
    D_80123044 = cur;

    btn = func_8006EC80(side);
    D_8010A5A4 = btn;

    /* When sub-panel is open, D-pad, Circle, and Cross are ignored. */
    if (D_8012304A[side] != 0) {
        if (btn == 0x4000U || btn == 0x1000U || btn == 0x8000U ||
            btn == 0x2000U || btn == 0x0020U || btn == 0x0040U) {
            return 0;
        }
    }

    btn = D_8010A5A4;
    if (btn == 0x0040U) {
        /* Cross: cancel active swap, or exit menu with return code 1 */
        func_800AB620(0x529);
        if (H(D_80123044, 0x2) == 0) {
            return 1;
        }
        func_80120974(side);
    } else if (btn == 0x0004U) {
        /* L1: next formation (0..19) */
        unsigned int form = B(Scratchpad, 0x2FD + side);
        B(Scratchpad, 0x2FD + side) = (form < 0x13U) ? (unsigned char)(form + 1U) : 0;
        func_800AB620(0x50D);
    } else if (btn == 0x0001U) {
        /* L2: cycle sub-panel option 0 (0..7) when sub-panel is open and idle */
        if (D_8012304A[side] != 0 && D_80122358[side] == 0) {
            unsigned int opt0 = B(Scratchpad, 0x301 + side);
            if (opt0 != B(Scratchpad, 0x303 + side)) {
                func_8012051C(side, 0, 0x5F);
            }
            B(Scratchpad, 0x301 + side) = (opt0 < 7U) ? (unsigned char)(opt0 + 1U) : 0;
            H(D_800FF748, (side + 0x1EU) * 40U + 0x5DD2U) = (unsigned short)(B(Scratchpad, 0x301 + side) << 4);
            func_8012051C(side, 0, 0x5E);
            func_800AB620(0x50D);
        }
    } else if (btn == 0x0002U) {
        /* R2: cycle sub-panel option 1 (0..7) when sub-panel is open and idle */
        if (D_8012304A[side] != 0 && D_80122358[side] == 0) {
            unsigned int opt1 = B(Scratchpad, 0x303 + side);
            if (B(Scratchpad, 0x301 + side) != opt1) {
                func_8012051C(side, 1, 0x5F);
            }
            B(Scratchpad, 0x303 + side) = (opt1 < 7U) ? (unsigned char)(opt1 + 1U) : 0;
            H(D_800FF748, (side + 0x20U) * 40U + 0x5DD2U) = (unsigned short)(B(Scratchpad, 0x303 + side) << 4);
            func_8012051C(side, 1, 0x5E);
            func_800AB620(0x50D);
        }
    } else if (btn == 0x0010U) {
        /* Triangle: toggle display flag Scratchpad+0x307[side] */
        unsigned char *spr = func_8001D004(0x1005);
        unsigned int flag = B(Scratchpad, 0x307 + side) ^ 1U;
        unsigned char *entry = spr + side * 12U;
        B(Scratchpad, 0x307 + side) = (unsigned char)flag;
        if (flag != 0) {
            entry[1] = 0x30;
            entry[3] = 0x80;
            entry[4] = 0x20;
            S(entry, 6) = (signed short)(side * 78U - 0x3CU);
        } else {
            entry[1] = 0x20;
            entry[3] = 0xE0;
            entry[4] = 0x80;
            S(entry, 6) = (signed short)(side * 78U - 0x37U);
        }
        func_800AB620(0x50D);
    } else if (btn == 0x0008U) {
        /* R1: previous formation (0..19) */
        unsigned int form = B(Scratchpad, 0x2FD + side);
        B(Scratchpad, 0x2FD + side) = (form != 0) ? (unsigned char)(form - 1U) : 0x13;
        func_800AB620(0x50D);
    } else if (btn == 0x0020U) {
        /* Circle: begin or confirm player swap */
        cur = D_80123044;
        if (H(cur, 0x2) == 0) {
            H(cur, 0x2) = 1;
            H(cur, 0xA) = H(cur, 0x0);
            func_800AB620(0x528);
        } else {
            unsigned int cursor;
            unsigned int swap_src;
            int valid = 1;

            team_base = side * 88U;
            squad = D_800FF748 + 0x8E9CU + side * 22U;

            if (B(Scratchpad, 0x2A0) == 1 && B(Scratchpad, 0x2E1) != 5) {
                if (B(Scratchpad, 0x29A) != 0 || B(Scratchpad, 0x2DF) != 0 ||
                    (W(Scratchpad, 0x2D0) & 0x00FFFF00U) != 0) {
                    if (func_80121A2C(side) >= 3U) {
                        cur = D_80123044;
                        cursor = H(cur, 0x0);
                        if (cursor >= 0xBU && D_800EF78B[team_base + squad[cursor] * 4U] == 0) {
                            valid = 0;
                        }
                        if (valid) {
                            swap_src = H(cur, 0xA);
                            if (swap_src >= 0xBU && D_800EF78B[team_base + squad[swap_src] * 4U] == 0) {
                                valid = 0;
                            }
                        }
                    }
                }
            }

            if (valid) {
                cur = D_80123044;
                cursor = H(cur, 0x0);
                swap_src = H(cur, 0xA);

                if ((D_800EF78B[team_base + squad[cursor] * 4U] & 0x10U) != 0 ||
                    (D_800EF78B[team_base + squad[swap_src] * 4U] & 0x10U) != 0 ||
                    swap_src == cursor) {
                    valid = 0;
                }
            }

            if (valid) {
                /* GK/role compatibility check (unless moving D_80123048 marker into starting XI) */
                if (!(D_80123048 != 0 && swap_src == (unsigned int)(D_80123048 - 1U) &&
                      cursor < 0xBU && D_80123049 == side)) {
                    unsigned int role_cur = D_800EF789[team_base + squad[cursor] * 4U];
                    unsigned int role_src = D_800EF789[team_base + squad[swap_src] * 4U];
                    if ((role_cur == 0) != (role_src == 0)) {
                        valid = 0;
                    }
                }
            }

            if (valid) {
                unsigned int avail_src = D_800FF748[0x88A5U + side * 22U + squad[swap_src]];
                unsigned int avail_cur = D_800FF748[0x88A5U + side * 22U + squad[cursor]];
                if (!((avail_src < 2U || cursor < 0xBU) && (avail_cur < 2U || swap_src < 0xBU))) {
                    valid = 0;
                }
            }

            if (valid && (unsigned int)(B(Scratchpad, 0x2E1) - 1U) < 2U) {
                unsigned int team_id = B(Scratchpad, 0x2DD + side);
                if (D_800FF748[0x93A9U + team_id * 132U + squad[swap_src] * 6U] == 1 ||
                    D_800FF748[0x93A9U + team_id * 132U + squad[cursor] * 6U] == 1) {
                    valid = 0;
                }
            }

            if (valid) {
                unsigned int avail_src = D_800FF748[0x88A5U + side * 22U + squad[swap_src]];
                unsigned int avail_cur = D_800FF748[0x88A5U + side * 22U + squad[cursor]];
                if (avail_src >= 2U || avail_cur >= 2U) {
                    if (D_80123048 != 0 &&
                        (swap_src == (unsigned int)(D_80123048 - 1U) || cursor == (unsigned int)(D_80123048 - 1U)) &&
                        D_80123049 == side) {
                        valid = 0;
                    }
                }
            }

            if (!valid) {
                func_80120974(side);
                func_800AB620(0x512);
            } else if (D_80123048 != 0 && D_80123049 == side &&
                       cursor < 0xBU && swap_src == (unsigned int)(D_80123048 - 1U)) {
                /* Reassign D_80123048 marker to cursor slot without swapping squad entries */
                D_80123048 = (unsigned char)(cursor + 1U);
                func_80120974(side);
                func_800AB620(0x528);
            } else {
                unsigned char tmp;
                H(cur, 0x2) = 0;
                tmp = squad[cursor];
                squad[cursor] = squad[swap_src];
                squad[swap_src] = tmp;

                if (D_80123048 != 0 && D_80123049 == side &&
                    cursor == (unsigned int)(D_80123048 - 1U) && swap_src < 0xBU) {
                    D_80123048 = (unsigned char)(swap_src + 1U);
                }

                if (B(Scratchpad, 0x2A0) == 1 && B(Scratchpad, 0x2E1) != 5) {
                    if (B(Scratchpad, 0x29A) != 0 || B(Scratchpad, 0x2DF) != 0 ||
                        (W(Scratchpad, 0x2D0) & 0x00FFFF00U) != 0) {
                        func_80121A2C(side);
                    }
                }

                func_80121670(side);
                func_80120A58(side);
                func_80120698(side);
                func_801211E8(other_side & 0xFFU, side);
                func_800AB620(0x528);
            }
        }
    } else if (btn == 0x1000U) {
        /* D-pad Up: move cursor up / scroll list up */
        cur = D_80123044;
        if (H(cur, 0x0) != H(cur, 0xC)) {
            H(cur, 0x0) -= 1;
            func_800AB620(0x50D);
        } else if (H(cur, 0x0) != 0) {
            H(cur, 0xC) -= 1;
            H(cur, 0xE) -= 1;
            H(cur, 0x0) -= 1;
            func_80120A58(side);
            if (H(D_80123044, 0xC) != 0) {
                func_8006FAF4(side + 1U, 1, 0xA);
            } else {
                func_8006FAF4(side + 1U, 0, 0x5B);
            }
            func_800AB620(0x50D);
        }
    } else if (btn == 0x0100U) {
        /* Select: toggle sub-panel slide if animation counter is idle */
        if (D_80122358[side] == 0) {
            D_8012304A[side] ^= 1;
            D_80122358[side] = 8;
            func_8006FA84(side + 3U, D_8012304A[side] ^ 1U);
        }
        func_800AB620(0x50D);
    } else if (btn == 0x0080U) {
        /* Square: cycle display mode D_800FF748+0xABDC[side] (0..2) */
        unsigned char *mode_ptr = D_800FF748 + 0xABDCU + side;
        unsigned char *spr;
        unsigned char *entry;
        unsigned int mode = *mode_ptr;

        *mode_ptr = (mode < 2U) ? (unsigned char)(mode + 1U) : 0;
        spr = func_8001D004(0x1004);
        entry = spr + side * 12U;
        mode = *mode_ptr;
        entry[1] = D_80122340[mode * 4U];
        entry[3] = D_80122341[mode * 4U];
        entry[4] = D_80122342[mode * 4U];
        S(entry, 6) = (signed short)(D_80122343[mode * 4U] - 0x40 + (signed int)(side * 78U));

        func_801211E8(other_side & 0xFFU, side);
        func_8012199C(side, (unsigned char)H(D_80123044, 0x0));
        func_800AB620(0x50D);
    } else if (btn == 0x0800U) {
        /* Start: close sub-panel if open, play SFX, and exit with code 2 */
        if (D_8012304A[side] != 0) {
            func_8006FA84(side + 3U, 1);
        }
        func_800AB620(0x528);
        return 2;
    } else if (btn == 0x4000U) {
        /* D-pad Down: move cursor down / scroll list down */
        cur = D_80123044;
        if (H(cur, 0x0) != H(cur, 0xE)) {
            H(cur, 0x0) += 1;
            func_800AB620(0x50D);
        } else if (H(cur, 0x0) < 0x15U) {
            H(cur, 0xC) += 1;
            H(cur, 0xE) += 1;
            H(cur, 0x0) += 1;
            func_80120A58(side);
            if (H(D_80123044, 0xE) == 0x15U) {
                func_8006FAF4(side + 1U, 1, 0x5B);
            } else {
                func_8006FAF4(side + 1U, 0, 0xA);
            }
            func_800AB620(0x50D);
        }
    } else if (btn == 0x2000U) {
        /* D-pad Right: jump scroll window to reserves (11..21) */
        cur = D_80123044;
        if (H(cur, 0xC) < 0xBU) {
            unsigned char *spr;
            H(cur, 0xC) = 0xB;
            H(cur, 0xE) = 0x15;
            H(cur, 0x0) = 0x15;
            spr = func_8001D004(side + 0x6AU);
            spr[0x0A] = 0xA;
            spr[0x16] = 0x5B;
            func_800AB620(0x50D);
        }
    } else if (btn == 0x8000U) {
        /* D-pad Left: jump scroll window to starters (0..10) */
        cur = D_80123044;
        if (H(cur, 0xC) != 0) {
            unsigned char *spr;
            H(cur, 0xC) = 0;
            H(cur, 0x0) = 0;
            H(cur, 0xE) = 0xA;
            spr = func_8001D004(side + 0x6AU);
            spr[0x0A] = 0x5B;
            spr[0x16] = 0xA;
            func_800AB620(0x50D);
        }
    }

    btn = D_8010A5A4;
    if (btn == 0x1000U || btn == 0x4000U) {
        func_80120698(side);
    }

    btn = D_8010A5A4;
    if (btn == 0x8000U || btn == 0x2000U) {
        func_80120698(side);
        func_80120A58(side);
        btn = D_8010A5A4;
    }

    if (btn == 0x0004U || btn == 0x0008U) {
        /* Refresh formation positions and player rows after L1/R1 */
        func_8012059C(side);
        func_801211E8(other_side & 0xFFU, side);

        cur = D_80123044;
        i = H(cur, 0xC);
        team_base = side * 88U;
        squad = D_800FF748 + 0x8E9CU + side * 22U;
        while (i <= H(cur, 0xE)) {
            unsigned int pos;
            if (i >= 0xBU) {
                pos = D_800EF789[team_base + squad[i] * 4U];
            } else {
                pos = D_800CA30C[B(Scratchpad, 0x2FD + side) * 11U + i];
            }
            func_8006FB90(side + 0xCU, i - H(cur, 0xC), pos << 4);
            i = (i + 1U) & 0xFFFFU;
            cur = D_80123044;
        }

        func_80121670(side);
        func_8012199C(side, (unsigned char)H(D_80123044, 0x0));
    }

    /* Swap source blink and animated arrow indicators */
    cur = D_80123044;
    if (H(cur, 0x2) != 0) {
        if (H(cur, 0xA) != H(cur, 0x0) && (SW(Scratchpad, 0x290) % 5) == 0) {
            unsigned int swap_src = H(cur, 0xA);
            unsigned int scroll_start = H(cur, 0xC);
            unsigned int scroll_end = H(cur, 0xE);
            unsigned int blink = H(cur, 0x4) ^ 1U;
            H(cur, 0x4) = (unsigned short)blink;
            if (blink != 0) {
                if (swap_src >= scroll_start && swap_src <= scroll_end) {
                    func_8006FAF4(side + 0xEU, swap_src - scroll_start, 0xB);
                }
                func_8006FAF4(side + 0x1AU, H(D_80123044, 0xA), 0x16);
            } else {
                if (swap_src >= scroll_start && swap_src <= scroll_end) {
                    func_8006FAF4(side + 0xEU, swap_src - scroll_start, 8);
                }
                func_8006FAF4(side + 0x1AU, H(D_80123044, 0xA), 0x17);
            }
        }

        if (H(D_80123044, 0x2) != 0) {
            func_8006F528((unsigned char)(side + 0x10U), D_8012234C, 3, 1);
            func_8006F528((unsigned char)((other_side & 0xFFU) + 0x1CU), D_80122350, 2, 2);

            if (D_80123048 != 0 && side == D_80123049 &&
                (SW(Scratchpad, 0x290) % 5) == 0 &&
                H(D_80123044, 0xA) == (unsigned int)(D_80123048 - 1U)) {
                func_8006FAF4(7, 0, D_80122354[D_80123011]);
                D_80123011 = (D_80123011 == 0) ? 1 : 0;
            }
        }
    }

    /* Scroll arrow animation toggle */
    if (H(D_80123044, 0x6) == 0) {
        if (func_8006F714(side + 1U, D_800CA2F8, 4) != 0) {
            H(D_80123044, 0x6) ^= 1;
        }
    } else {
        if (func_8006F624(side + 1U, D_800CA2FC, 4) != 0) {
            H(D_80123044, 0x6) ^= 1;
        }
    }

    /* Sub-panel open/close slide animation (D_80122358[side] > 0) */
    if (D_80122358[side] != 0) {
        signed int delta = (D_8012304A[side] != 0) ? 0x20 : -0x20;
        signed int panel_delta = (D_8012304A[side] != 0) ? 0x40 : -0x40;
        unsigned char *spr;

        for (i = 0; i < 7U; i++) {
            unsigned char *entry = D_800FF748 + 0x5DC0U + (side + 0xCU + i * 2U) * 40U;
            if (entry[4] != 0) {
                S(entry, 0x12) += (signed short)delta;
            }
        }

        {
            unsigned char *entry = D_800FF748 + 0x5DE8U + side * 40U; /* D_80105508[side + 1] */
            if (entry[4] != 0) {
                S(entry, 0x12) += (signed short)delta;
            }
        }

        spr = func_8001D004(0x100F);
        S(spr + side * 12U, 0x8) += (signed short)delta;
        S(spr + side * 12U + 0x18U, 0x8) += (signed short)delta;

        S(D_800FF748, 0xAC2CU + side * 2U) += (signed short)panel_delta;

        for (i = 0; i < 3U; i++) {
            unsigned char *entry = D_800FF748 + 0x5DC0U + (side + 0x1EU + i * 2U) * 40U;
            if (entry[4] != 0) {
                S(entry, 0x12) += (signed short)delta;
            }
        }

        if ((B(Scratchpad, 0x2AD) / 12U) == (other_side & 0xFFU)) {
            S(D_800FF748, 0x5EEA) += (signed short)delta; /* D_80105508[7] +0x12 */
        }

        D_80122358[side] -= 1;
    }

    return 0;
}
