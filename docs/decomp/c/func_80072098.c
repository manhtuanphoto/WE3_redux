/*
 * func_80072098
 *
 * Purpose:
 *   Menu/sub-screen state machine (likely match options / settings / formation
 *   overlay menu). Dispatches on D_8010A321 (menu phase 0..3) to initialize
 *   menu widgets (via func_80071AF8 / func_8001D004), process D-pad / confirm
 *   input (updating D_800F109C cursor row, D_8010C1D0 option bitmask, and
 *   sprite attributes), and transition between menu phases.
 *
 * Arguments:
 *   a0 (unsigned char) - menu/screen id passed to func_80071AF8 / func_80071D94 / func_80071F24
 *   a1 (unsigned char) - item count / parameter passed to sub-menu helpers
 *
 * Return value:
 *   int - 1 when confirm (0x20) triggers the special exit path in phase 2
 *         (D_8010A320 == 5 && D_800F109C == 0), 0 otherwise.
 *
 * Reads / Writes:
 *   Scratchpad (0x1F800000):
 *     +0x2A1 (u8) mode/flag checked before applying option flags (R)
 *     +0x2CE (u8) flag; when 0 and D_8010A320 == 2, syncs options to globals (R)
 *     +0x2E1 (u8) mode checked in phase 2 button 0x20 handler (R)
 *     +0x2E6 (u8) game mode/count; >= 5 restricts option bits 0..1 (R)
 *   Globals:
 *     D_800F109C (u8)   selected menu row/cursor (R/W)
 *     D_80105B9C (u8)   option bit 3 flag (D_800FF748 + 0x6454) (W)
 *     D_80105BC4 (u8)   option bit 4 flag (D_800FF748 + 0x647C) (W)
 *     D_80105C14 (u8)   option bit 2 flag (D_800FF748 + 0x64CC) (W)
 *     D_80105C3C (u8)   option bit 2 flag (D_800FF748 + 0x64F4) (W)
 *     D_80105CB4 (u8)   option bit 2 flag (D_800FF748 + 0x656C) (W)
 *     D_8010A320 (u8)   menu type / sub-mode (1, 2, 5, 6) (R/W)
 *     D_8010A321 (u8)   menu state machine phase (0..3) (R/W)
 *     D_8010A326 (u8)   saved cursor row for mode 6 (R/W)
 *     D_8010A327 (u8)   saved option bitmask for mode 6 (R/W)
 *     D_8010A5A8 (u8[]) settings struct (+0xA: saved option bitmask) (R/W)
 *     D_8010A5B2 (u8)   initial option bitmask for mode 2 (R)
 *     D_8010C1D0 (u8)   active option bitmask (R/W)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   Menu/UI function (no world-space units).
 *   Pad masks: 0x20 (Circle/Confirm?), 0x40 (Cross/Cancel?), 0x1000 (Up),
 *              0x2000 (Right), 0x4000 (Down), 0x8000 (Left).
 *   Sound IDs: 0x50D (cursor move/toggle), 0x528 (confirm), 0x529 (close/back).
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))

extern unsigned char D_800F109C;
extern unsigned char D_800FF748[];
extern unsigned char D_8010A320;
extern unsigned char D_8010A321;
extern unsigned char D_8010A326;
extern unsigned char D_8010A327;
extern unsigned char D_8010A5A8[];
extern unsigned char D_8010A5B2;
extern unsigned char D_8010C1D0;

extern unsigned char *func_8001D004(int id);
extern unsigned int func_8006EC80(unsigned char pad);
extern void func_8006F4A8(int id, int y);
extern void func_8006FCFC(int id);
extern unsigned int func_8006FFC4(void);
extern void func_80071AF8(unsigned char a0, unsigned char a1, unsigned char a2, int a3, int a4);
extern unsigned char func_80071D94(unsigned char a0, unsigned char a1);
extern unsigned char func_80071F24(unsigned char a0, unsigned char a1);
extern void func_80072904(void);
extern void func_800AB620(int sfx_id);

int func_80072098(unsigned char a0, unsigned char a1)
{
    unsigned char *scratch = (unsigned char *)0x1F800000;
    int ret = 0;

    switch (D_8010A321) {
    case 0: {
        /* UNCLEAR: a2 is left uninitialized in asm if D_8010A320 is not 1, 2, or 6 */
        unsigned char a2 = 0;
        unsigned char *sprite;
        unsigned char i;
        int cursor_y;

        switch (D_8010A320) {
        case 1:
            D_800F109C = 1;
            a2 = 0x64;
            break;
        case 2:
            D_800F109C = 0;
            D_8010C1D0 = D_8010A5B2;
            a2 = 0x62;
            break;
        case 6:
            D_800F109C = D_8010A326;
            D_8010C1D0 = D_8010A327;
            a2 = 0x63;
            break;
        }

        func_80071AF8(a0, a1, a2, 0x65, 4);

        if (D_8010A320 == 2) {
            sprite = func_8001D004(0x62);
            if (B(scratch, 0x2E6) >= 5 && (B(D_8010A5A8, 0xA) & 3) != 0) {
                B(sprite, 0x4) = 0xC0;
            } else {
                B(sprite, 0x4) = (unsigned char)(((B(D_8010A5A8, 0xA) & 3) << 4) - 0x70);
            }

            for (i = 1; i < 5; i++) {
                unsigned char *entry = sprite + i * 12;
                unsigned char bit = (unsigned char)((B(D_8010A5A8, 0xA) >> (i + 1)) & 1);
                B(entry, 0x4) = (unsigned char)(-0x70 - (bit << 4));
            }
        }

        if (D_8010A320 == 6) {
            sprite = func_8001D004(0x63);
            B(sprite, 0x28) = (unsigned char)(((D_8010A327 & 1) << 4) - 0x80);
            B(sprite, 0x34) = (unsigned char)(((D_8010A327 >> 1) << 4) - 0x40);
        }

        if (D_8010A320 == 5) {
            cursor_y = (D_800F109C << 4) - 8;
        } else {
            cursor_y = ((D_800F109C << 4) - 0xB) - ((a1 - 1) << 3);
        }
        func_8006F4A8(2, cursor_y);
        D_8010A321++;
        break;
    }

    case 1:
        if (func_80071D94(a0, a1) == 0) {
            D_8010A321++;
        }
        break;

    case 2: {
        signed short base_y;
        unsigned char item_count;
        unsigned int pad;
        unsigned char row;
        unsigned char *sprite;

        func_8006FCFC(0x36);
        if (D_8010A320 == 5) {
            base_y = 0;
            item_count = 2;
        } else {
            base_y = -0xB;
            item_count = a1;
        }

        pad = func_8006EC80((unsigned char)func_8006FFC4());
        switch (pad) {
        case 0x1000: /* Up */
            row = D_800F109C;
            if (row != 0) {
                row--;
            } else {
                row = item_count - 1;
            }
            D_800F109C = row;
            func_8006F4A8(2, (base_y - ((item_count - 1) << 3)) + (row << 4));
            func_800AB620(0x50D);
            break;

        case 0x4000: /* Down */
            row = D_800F109C;
            if ((int)row < (int)item_count - 1) {
                row++;
            } else {
                row = 0;
            }
            D_800F109C = row;
            func_8006F4A8(2, (base_y - ((item_count - 1) << 3)) + (row << 4));
            func_800AB620(0x50D);
            break;

        case 0x2000: /* Right */
        case 0x8000: /* Left */
            if (D_8010A320 == 2) {
                sprite = func_8001D004(0x62);
                row = D_800F109C;
                if (row == 0) {
                    unsigned char old_bits = D_8010C1D0;
                    unsigned char cur_val = old_bits & 3;
                    unsigned char new_val;

                    if (B(scratch, 0x2E6) >= 5) {
                        if (cur_val == 0) {
                            D_8010C1D0 = old_bits | 2;
                        }
                        if (cur_val != 0) {
                            new_val = 0;
                        } else {
                            new_val = D_8010C1D0 & 3;
                        }
                    } else {
                        unsigned int dir = func_8006EC80((unsigned char)func_8006FFC4());
                        if (dir == 0x8000) {
                            if (cur_val != 0) {
                                new_val = cur_val - 1;
                            } else {
                                new_val = 2;
                            }
                        } else if (cur_val < 2) {
                            new_val = cur_val + 1;
                        } else {
                            new_val = 0;
                        }
                    }

                    if (B(scratch, 0x2E6) >= 5 && new_val != 0) {
                        B(sprite, 0x4) = 0xC0;
                    } else {
                        B(sprite, 0x4) = (unsigned char)((new_val << 4) - 0x70);
                    }
                    D_8010C1D0 = (unsigned char)((D_8010C1D0 & 0xFC) | new_val);
                } else {
                    int bit_idx = row + 1;
                    unsigned char bits = D_8010C1D0 ^ (unsigned char)(1 << bit_idx);
                    D_8010C1D0 = bits;
                    B(sprite + row * 12, 0x4) = (unsigned char)(-0x70 - (((bits >> bit_idx) & 1) << 4));
                }
                func_800AB620(0x50D);
            }
            if (D_8010A320 == 6 && D_800F109C != 0) {
                sprite = func_8001D004(0x63);
                row = D_800F109C;
                if (row == 1) {
                    D_8010C1D0 ^= 1;
                    B(sprite, 0x28) = (unsigned char)(((D_8010C1D0 & 1) << 4) - 0x80);
                } else {
                    D_8010C1D0 ^= 2;
                    B(sprite + row * 12, 0x1C) = (unsigned char)(((D_8010C1D0 >> 1) << 4) - 0x40);
                }
                func_800AB620(0x50D);
            }
            break;

        case 0x20: /* Confirm */
            if (D_8010A320 == 5 && D_800F109C == 0) {
                if (B(scratch, 0x2E1) == 5) {
                    sprite = func_8001D004(2);
                    B(sprite, 0xA) = 3;
                }
                ret = 1;
            }
            func_800AB620(0x528);
            /* fallthrough to close/apply */

        case 0x40: /* Back / Close */
            if (D_8010A320 == 6) {
                D_8010A326 = D_800F109C;
                D_8010A327 = D_8010C1D0;
            }
            func_800AB620(0x529);
            D_8010A321++;
            break;
        }

        if (B(scratch, 0x2CE) == 0 && D_8010A320 == 2) {
            unsigned char opts = D_8010C1D0;
            B(D_8010A5A8, 0xA) = opts;
            if (B(scratch, 0x2A1) == 1) {
                unsigned char i;
                /* D_80105B9C, D_80105BC4, D_80105C14, D_80105CB4, D_80105C3C */
                B(D_800FF748, 0x6454) = (unsigned char)((opts & 8) >> 3);
                B(D_800FF748, 0x647C) = (unsigned char)((B(D_8010A5A8, 0xA) & 0x10) >> 4);
                B(D_800FF748, 0x64CC) = (unsigned char)((B(D_8010A5A8, 0xA) & 4) >> 2);
                B(D_800FF748, 0x656C) = (unsigned char)((B(D_8010A5A8, 0xA) & 4) >> 2);
                B(D_800FF748, 0x64F4) = (unsigned char)((B(D_8010A5A8, 0xA) & 4) >> 2);
                for (i = 0; i < 2; i++) {
                    sprite = func_8001D004(i);
                    if ((B(D_8010A5A8, 0xA) & 0x20) != 0) {
                        B(sprite, 0x28) = 0xD0;
                        B(sprite, 0x26) = 0x10;
                        B(sprite, 0x34) = 0xE0;
                        B(sprite, 0x32) = 0x10;
                    } else {
                        B(sprite, 0x28) = 0xC8;
                        B(sprite, 0x26) = 0x08;
                        B(sprite, 0x34) = 0xC8;
                        B(sprite, 0x32) = 0x08;
                    }
                }
            }
            func_80072904();
        }
        break;
    }

    case 3:
        if (func_80071F24(a0, a1) == 0) {
            D_8010A320 = 0;
            D_8010A321 = 2;
        }
        break;
    }

    return ret;
}
