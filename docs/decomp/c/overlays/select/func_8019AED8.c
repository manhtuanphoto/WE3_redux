/*
 * func_8019AED8 (overlay: select)
 *
 * Purpose:
 *   Sub-state machine for the match options / stadium / rules menu inside the
 *   `select` overlay. Reads sub-state D_8010A322 (u8); if `< 0x33` (51),
 *   dispatches via `jtbl_80189DAC[substate]` to initialize option defaults,
 *   animate transitions between option rows (cursor D_800AD638: 0..4, skipping
 *   row 2), handle pad 0 inputs for each option row, and exit back or forward
 *   via func_800171FC.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 */

#define SCRATCHPAD_BASE 0x1F800000U

extern unsigned char D_800FF748[];
extern unsigned char D_8010A322;   /* Sub-state index (0..50) */
extern unsigned short D_800AD638;  /* Selected menu row index (0..4) */
extern unsigned short D_8010A5A4;  /* Last polled button mask from pad 0 */
extern unsigned char D_801DAE50;   /* Confirm/cancel direction flag (0, 1, 2) */
extern signed char D_801D8C60;     /* Row scroll direction (-1, 0, +1) */
extern unsigned char D_801D8FB8;
extern unsigned char D_801D8DF4;
extern unsigned char D_801D8DF5;
extern unsigned char D_801D8DF6;
extern unsigned char D_801D8DF7;
extern unsigned char D_801D8DF8;
extern unsigned char D_801D8DF9;
extern unsigned char D_801D8DFA;
extern unsigned char D_801D8C64;
extern unsigned char D_801D8C65;
extern unsigned char D_801D8C66;
extern unsigned char D_801D8C67;
extern unsigned char D_801DA2F4[]; /* Option values per row (D_801DA2F4..D_801DA2F7) */
extern unsigned char D_801DA2F5;
extern unsigned char D_801DA2F6;
extern unsigned char D_801DA2F7;
extern unsigned int D_801DA058;
extern unsigned int D_801DA05C;
extern unsigned int D_801DA060;
extern unsigned int D_801DA064;
extern unsigned int D_8011D668[];
extern unsigned int D_8011D694;
extern unsigned int D_8011D6BC[];
extern unsigned int D_8011D6DC;
extern unsigned char D_801D92BF;
extern unsigned char D_801D92DB;

/* Jump table in .rodata (raw entries not present in compact .s) */
extern void *jtbl_80189DAC[];

extern void func_8019C43C(void);
extern int func_8018E720(int arg0, int arg1);
extern int func_8019C61C(void);
extern void func_8018E4AC(int arg0, int arg1, int arg2);
extern void func_8019DF70(void);
extern void func_8019E25C(int arg0);
extern void func_80193FB4(int id, int arg1, int arg2, int arg3,
                          int frame, int h, int arg6, int sx,
                          int sy, int arg9, int arg10, int r,
                          int g, int b, int arg14, int arg15, int arg16);
extern void func_8006FA60(int slot, int val);
extern void func_8006FA84(int slot, int val);
extern void func_8018EC60(int slot, unsigned int text_ptr);
extern void func_8019D23C(int arg0);
extern void func_8019D75C(int arg0);
extern void func_8019D904(int arg0);
extern void func_8019CD0C(void);
extern int func_8019CB54(void);
extern void func_8019C85C(void);
extern unsigned short func_8006EC80(int pad_mode);
extern void func_8006FCFC(int clut_idx);
extern void func_800AB620(int audio_cmd);
extern int func_8018E5EC(int cur, int min_val, int max_val);
extern void func_8019DDA8(int arg0);
extern int func_8019C73C(void);
extern void func_800171FC(int next_state);

void func_8019AED8(void)
{
    volatile unsigned char *sp_2e2 = (volatile unsigned char *)(SCRATCHPAD_BASE + 0x2E2);

    /* Globals accessed relative to D_800FF748 */
    volatile unsigned short *g_80105592 = (volatile unsigned short *)(D_800FF748 + 0x5E4A);
    volatile unsigned char *g_80105584  = (volatile unsigned char *)(D_800FF748 + 0x5E3C);
    volatile unsigned char *g_801055D4  = (volatile unsigned char *)(D_800FF748 + 0x5E8C);
    volatile unsigned char *g_80105CB4  = (volatile unsigned char *)(D_800FF748 + 0x656C);
    volatile unsigned char *g_8010A322  = (volatile unsigned char *)(D_800FF748 + 0xABDA); /* +0x10000 - 0x5426 */

    unsigned int substate = D_8010A322;
    int ready = 1;

    if (substate >= 0x33U) {
        return;
    }

    /*
     * UNCLEAR: Exact mapping of `substate` (0..0x32) via jtbl_80189DAC[substate] is in
     * .rodata and not present in the .s file. Several state values are observable from
     * explicit transitions below:
     *   - 0x00: .L8019AF2C (initializes defaults and increments D_8010A322)
     *   - 0x03: .L8019B050 (sets up UI for row D_800AD638 and jumps to 0x14 + row)
     *   - 0x04: .L8019B294 (sets D_8010A322 = D_801DAE50 + 5)
     *   - 0x0A: .L8019B3BC (starts row scroll animation)
     *   - 0x14: .L8019B4C8 (row 0 input handler)
     *   - 0x15 / 0x17: .L8019B648 (row 1 / row 3 input handler)
     *   - 0x18: .L8019B78C (row 4 input handler)
     *   - 0x28: .L8019B7FC (exit forward to parent state 3)
     *   - 0x32: .L8019B830 (exit back to parent state 1)
     */
    (void)jtbl_80189DAC[substate];

    /* .L8019AF2C (substate 0): Initialize match-option defaults */
    if (0) {
        D_800AD638 = 0;
        D_801DAE50 = 0;
        D_801D8C60 = 0;
        *sp_2e2 = 0;

        D_801D8DF4 = 6;
        D_801D8DF7 = 0x10;
        D_801D8DF5 = 1;
        D_801D8DF6 = 1;
        D_801D8DF8 = 0;
        D_801D8DF9 = 1;
        D_801D8DFA = 1;

        D_801D8C64 = 0;
        D_801D8C65 = 1;
        D_801D8C66 = 2;
        D_801D8C67 = 3;

        D_801DA2F4[0] = 0;
        D_801DA2F5 = 0;
        D_801DA2F6 = 0;
        D_801DA2F7 = 0;

        D_801DA064 = 0;
        D_801DA060 = 0;
        D_801DA05C = 0;
        D_801DA058 = 0;

        *g_8010A322 = (unsigned char)(*g_8010A322 + 1);
        return;
    }

    /* .L8019B018: Setup graphics and advance substate */
    if (0) {
        func_8019C43C();
        *g_8010A322 = (unsigned char)(*g_8010A322 + 1);
        return;
    }

    /* .L8019B028: Wait for intro/fade-in completion */
    if (0) {
        ready &= func_8018E720(1, 0);
        ready &= func_8019C61C();
        if (ready != 0) {
            *g_8010A322 = (unsigned char)(*g_8010A322 + 1);
        }
        return;
    }

    /* .L8019B050 (substate 3): Enter active option row D_800AD638 */
    if (0) {
        unsigned short row = D_800AD638;
        if (row != 4) {
            func_8018E4AC(0x26, 0x31, 0);
        }

        row = D_800AD638;
        if (row == 0) {
            func_8019DF70();
            func_8019E25C(D_801DA2F4[D_800AD638]);
            func_80193FB4(3, 0x301F, 0, 0x4D,
                          (int)(D_801DA2F4[0] * 34 + 0xF), 0x18, 0,
                          0x1000, 0x1000, 0, 0, 0x80, 0x80, 0x80, 0, 2, 1);
            func_8006FA60(D_801DA2F4[0] + 0x26, 0x1F);
            func_8018EC60(0, D_8011D668[*sp_2e2]);
        } else if (row == 1) {
            func_8019D23C(0x0B);
            func_8019D75C(D_800AD638 + 0x0A);
            func_8019D904(D_800AD638 + 0x0A);
            func_8018EC60(0, D_8011D6BC[D_801DA2F4[D_800AD638]]);
        } else if (row == 3) {
            func_8019D23C(0x0D);
            func_8019D904(D_800AD638 + 0x0A);
            func_8018EC60(0, D_8011D694);
        } else if (row == 4) {
            func_8018EC60(0, D_8011D6DC);
            func_80193FB4(0x31, 0x3025, 0, 0x0A,
                          0, 0x18, 0,
                          0x1000, 0x1000, 0, 0, 0x80, 0x80, 0x80, 0, 2, 1);
        }

        *g_8010A322 = (unsigned char)((D_800AD638 & 0xFF) + 0x14);
        return;
    }

    /* .L8019B294 (substate 4): Branch to confirm/cancel transition */
    if (0) {
        *g_8010A322 = (unsigned char)(D_801DAE50 + 5);
        return;
    }

    /* .L8019B2B8: Handle confirm transition out of current row */
    if (0) {
        unsigned short row = D_800AD638;
        if (row == 4) {
            D_801D8FB8 = 0;
            func_8018E4AC(0x26, 0x31, 0);
            *g_8010A322 = 0x28;
        } else if (row == 3) {
            D_800AD638 = (unsigned short)(row + 1);
            *g_8010A322 = 3;
        } else {
            D_801D8C60 = -1;
            *g_8010A322 = 0x0A;
        }
        return;
    }

    /* .L8019B31C: Handle cancel transition out of current row */
    if (0) {
        unsigned short row = D_800AD638;
        if (row == 0) {
            D_801D8FB8 = 0;
            func_8018E4AC(0x26, 0x31, 0);
            *g_801055D4 = 0;
            *g_8010A322 = 0x32;
        } else if (row == 4) {
            *g_80105CB4 = 0;
            *g_8010A322 = 3;
            D_800AD638 = (unsigned short)(D_800AD638 - 1);
        } else {
            D_801D8C60 = 1;
            *g_8010A322 = 0x0A;
        }
        return;
    }

    /* .L8019B3BC (substate 0xA): Clear row sprites and start scroll */
    if (0) {
        func_8018E4AC(0x1E, 0x25, 0);
        func_8019CD0C();
        *g_8010A322 = (unsigned char)(*g_8010A322 + 1);
        return;
    }

    /* .L8019B3FC: Step row scroll animation and settle on next row */
    if (0) {
        if (func_8019CB54() == 0) {
            func_8019CD0C();
        } else {
            signed char dir = D_801D8C60;
            unsigned short row = D_800AD638;

            if (dir < 0) {
                row++;
            } else if (dir != 0) {
                row--;
            }
            D_800AD638 = row;

            if (row == 2) {
                /* Skip row 2 */
                signed char dir2 = D_801D8C60;
                if (dir2 < 0) {
                    D_800AD638 = 3;
                } else if (dir2 == 0) {
                    D_800AD638 = 2;
                } else {
                    D_800AD638 = 1;
                }
            }

            D_801D8C60 = 0;
            func_8018E4AC(0x14, 0x1B, 0);
            func_8019C85C();
            *g_8010A322 = 3;
        }
        return;
    }

    /* .L8019B4C8 (substate 0x14): Row 0 input handler */
    if (0) {
        unsigned short btn;
        D_801DAE50 = 0;
        btn = func_8006EC80(0);
        D_8010A5A4 = btn;
        func_8006FCFC(0x36);

        if (D_8010A5A4 == 0x1000 || D_8010A5A4 == 0x4000) {
            unsigned char val;
            func_8006FA60(D_801DA2F4[0] + 0x26, 0x1E);
            func_800AB620(0x50D);
            val = (unsigned char)func_8018E5EC(D_801DA2F4[0], 0, 2);
            func_8006FA60(val + 0x26, 0x1F);
            D_801DA2F4[0] = val;

            if (val == 0) {
                *sp_2e2 = 0;
            } else if (val == 1) {
                *sp_2e2 = 2;
            } else if (val == 2) {
                *sp_2e2 = 3;
            }

            *g_80105592 = (unsigned short)(D_801DA2F4[0] * 34 + 0xF);
            func_8018EC60(0, D_8011D668[*sp_2e2]);
        }

        if (D_8010A5A4 == 0x20) {
            func_800AB620(0x528);
            *g_80105584 = 0;
            D_801D8DF5 = 1;
            D_801D8DF6 = 1;
            D_801DAE50 = 2;
            *g_8010A322 = 4;
        }
        if (D_8010A5A4 == 0x40) {
            func_8006FA84(3, 0);
            func_800AB620(0x529);
            D_801DAE50 = 1;
            *g_8010A322 = 4;
        }
        return;
    }

    /* .L8019B648 (substate 0x15 / 0x17): Row 1 & Row 3 input handler */
    if (0) {
        unsigned short btn;
        D_801DAE50 = 0;
        btn = func_8006EC80(0);
        D_8010A5A4 = btn;

        if (btn == 0x1000 || btn == 0x4000) {
            if (D_800AD638 != 3) {
                func_800AB620(0x50D);
                D_801DA2F5 = (unsigned char)func_8018E5EC(D_801DA2F5, 0, 1);
                func_8018EC60(0, D_8011D6BC[D_801DA2F5]);
                func_8019D75C(0x0B);
            }
        }

        if (D_8010A5A4 == 0x8000 || D_8010A5A4 == 0x2000) {
            func_800AB620(0x50D);
            func_8019DDA8(D_800AD638 + 0x0A);
            func_8019D904(D_800AD638 + 0x0A);
        }

        if (D_8010A5A4 == 0x20) {
            func_800AB620(0x528);
            if (D_800AD638 == 1) {
                D_801D8DF9 = 0;
            }
            D_801DAE50 = 2;
            *g_8010A322 = 4;
        }
        if (D_8010A5A4 == 0x40) {
            func_800AB620(0x529);
            D_801DAE50 = 1;
            *g_8010A322 = 4;
        }
        return;
    }

    /* .L8019B78C (substate 0x18): Row 4 input handler */
    if (0) {
        unsigned short btn;
        func_8006FCFC(0x36);
        btn = func_8006EC80(0);
        D_8010A5A4 = btn;

        if (btn == 0x20) {
            func_800AB620(0x528);
            *g_80105CB4 = 0;
            D_801DAE50 = 2;
            *g_8010A322 = 4;
        } else if (btn == 0x40) {
            func_800AB620(0x529);
            *g_80105CB4 = 0;
            D_801DAE50 = 1;
            *g_8010A322 = 4;
        }
        return;
    }

    /* .L8019B7FC (substate 0x28): Confirm exit to parent state 3 */
    if (0) {
        ready &= func_8018E720(2, 0x3A);
        ready &= func_8019C73C();
        if (ready != 0) {
            func_8018E4AC(0x1E, 0x31, 0);
            D_801D92BF = 0;
            D_801D92DB = 0;
            func_800171FC(3);
            *g_8010A322 = 0;
        }
        return;
    }

    /* .L8019B830 (substate 0x32): Cancel exit to parent state 1 */
    if (0) {
        ready &= func_8019C73C();
        if (ready != 0) {
            func_8018E4AC(0x1E, 0x31, 0);
            D_801D92BF = 0;
            D_801D92DB = 0;
            func_800171FC(1);
            *g_8010A322 = 0;
        }
    }
}
