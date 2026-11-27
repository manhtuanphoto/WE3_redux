/*
 * func_8019E620 (overlay: select)
 *
 * Purpose:
 *   Main team-select screen state machine driven by D_8010A322
 *   (D_800FF748 + 0xABDA, states 0..32 via jtbl_8018A018).
 *   Handles team-select initialization (3D cloth flag meshes via func_8005C49C,
 *   unlock checks for special teams 40..43, default team clamping), asset/fade
 *   waiting, P1/P2 team cursor/UV updates, sequential (1P/CPU or mode 5) and
 *   simultaneous (Scratchpad+0x2E2 == 1) team confirmation/cancellation, and
 *   screen teardown returning 1 (back/cancel) or 2 (confirmed) when finished.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   0 while the team-select screen remains active (or state >= 33)
 *   1 on cancel/back completion (state 31)
 *   2 on confirm/forward completion (state 32)
 *
 * Global / table fields read & written:
 *   D_8010A322 (== D_800FF748 + 0xABDA, u8): state machine phase
 *   D_800FF748 + 0x5E08 / +0x5E30: side 0 / side 1 fade/animation structs (mode 5)
 *   D_800FF748 + 0x5E38 / +0x5E60 (u32): side 0 / side 1 palette/display mode (1 or 4 when teams match)
 *   D_800FF748 + 0x5E3C / +0x5E64 (u8): side 0 / side 1 active/confirmed flags
 *   D_800FF748 + 0x5E48,+0x5E4A (s16): side 0 flag sheet UV (col*31, row*22)
 *   D_800FF748 + 0x5E70,+0x5E72 (s16): side 1 flag sheet UV (col*31, row*22)
 *   D_800FF748 + 0x5FA4 (u8): cleared in state 0 when Scratchpad+0x2E1 == 5
 *   D_800FF748 + 0xABE2 (u8): unlock bitmask (bits 0..2 unlock teams 40..43)
 *   D_800FF748 + 0xAC20 / +0xAC21 (u8): 3D flag cloth mesh 0 / 1 active flags
 *   D_800FF748 + 0xAC28..+0xAC3C (s16): 3D flag cloth mesh params (D_8010A370..D_8010A384)
 *   D_801DAD88[44] (u8, 0x801DAD88..0x801DADB3): per-team availability/selected state (0=free, 1=picked, 3=locked)
 *   D_801DA058 / D_801DA05C (u32): P1 / P2 confirmed flags in simultaneous mode
 *   D_801DAE50 (u8): exit direction (1=cancel -> state 31, 2=confirm -> state 32)
 *
 * Scratchpad (0x1F800000) fields:
 *   +0x1EC (u8): cleared to 0 in state 0
 *   +0x290 (u32): bit 0x10 selects which side uses alternate kit/palette 4 on mirror match
 *   +0x2A2 (u8): set to 1 in state 0, cleared to 0 on exit
 *   +0x2DD (u8): side 0 (home/P1) selected team ID (0..43)
 *   +0x2DE (u8): side 1 (away/P2) selected team ID (0..43)
 *   +0x2E1 (u8): game mode (0, 1, 3, 5 checked)
 *   +0x2E2 (u8): player/match sub-mode (1 = simultaneous 2P selection)
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned char D_8010A322;
extern unsigned char D_801D1F60;
extern unsigned char D_801D1F61;
extern unsigned char D_801D803B;
extern unsigned char D_801D8198;
extern unsigned char D_801D8199;
extern unsigned char D_801D819A;
extern unsigned char D_801D819B;
extern unsigned char D_801D819C;
extern unsigned char D_801D819D;
extern unsigned char D_801D819E;
extern unsigned char D_801D819F;
extern unsigned char D_801D81A0;
extern unsigned char D_801D81A1;
extern unsigned char D_801D8B17;
extern unsigned char D_801D8FE8;
extern unsigned char D_801D9000;
extern unsigned char D_801D9018;
extern unsigned char D_801D932F;
extern unsigned int D_801DA058;
extern unsigned int D_801DA05C;
extern unsigned int D_801DA060;
extern unsigned int D_801DA064;
extern unsigned char D_801DAD88[];
extern unsigned char D_801DADB0;
extern unsigned char D_801DADB1;
extern unsigned char D_801DADB2;
extern unsigned char D_801DADB3;
extern unsigned char D_801DAE50;

extern void func_800501DC(int arg0);
extern void func_8005C49C(void);
extern void func_8006FA84(int slot, int active);
extern void func_8006FAA8(int slot, int arg1);
extern void func_8006FC08(unsigned int slot, unsigned int idx_from, unsigned int idx_to);
extern void func_80096050(void);
extern void func_800AB620(int sfx);
extern void func_8018E4AC(int arg0, int arg1, int arg2);
extern unsigned int func_8018E69C(int side);
extern void func_8018E720(int arg0, int arg1);
extern unsigned int func_8018E994(unsigned char *ptr, int arg1, int arg2);
extern void func_80193FB4(
    int a0, int a1, int a2, int a3,
    int s0, int s1, int s2, int s3,
    int s4, int s5, int s6, int s7,
    int s8, int s9, int s10, int s11, int s12);
extern void func_801A2610(void);
extern unsigned int func_801A29FC(void);
extern unsigned int func_801A2B3C(void);
extern void func_801A2CA0(int arg0, int arg1);
extern void func_801A2F90(int arg0, int arg1);
extern void func_801A30F4(int arg0, int arg1);
extern void func_801A346C(int arg0, int arg1);
extern void func_801A3588(void);
extern unsigned int func_801A39E4(int side, int arg1, int locked, int arg3);
extern void func_801A5764(unsigned int p1, unsigned int p2);
extern void func_801A61FC(int side, int arg1, int arg2);
extern void func_801A62EC(void);
extern void func_801A63A8(void);

/*
 * Computes flag sheet UV coordinates for a team ID (11 columns x 4 rows,
 * 31x22 px per cell) and stores them as s16 pair at D_800FF748 + offset.
 */
static void set_team_flag_uv(int offset, unsigned char team_id) {
    unsigned int row = team_id / 11;
    unsigned int col = team_id % 11;

    S(D_800FF748, offset)     = (signed short)(col * 31);
    S(D_800FF748, offset + 2) = (signed short)(row * 22);
}

/*
 * Updates side 0/1 palette mode (1 vs 4) when both sides select the same team.
 */
static void update_mirror_match_palette(int require_side1_active) {
    if (B(Scratchpad, 0x2DD) == B(Scratchpad, 0x2DE) &&
        (!require_side1_active || B(D_800FF748, 0x5E64) != 0)) {
        if ((W(Scratchpad, 0x290) & 0x10) != 0) {
            W(D_800FF748, 0x5E38) = 1;
            W(D_800FF748, 0x5E60) = 4;
        } else {
            W(D_800FF748, 0x5E38) = 4;
            W(D_800FF748, 0x5E60) = 1;
        }
    } else {
        W(D_800FF748, 0x5E38) = 1;
        W(D_800FF748, 0x5E60) = 1;
    }
}

/*
 * Rotates CLUT entries 1..13 across slots 0x54..0x56 for animated menu highlights.
 */
static void cycle_menu_cluts(void) {
    if (B(Scratchpad, 0x2E1) != 3) {
        func_8006FC08(0x54, 1, 0xD);
        func_8006FC08(0x55, 1, 0xD);
        func_8006FC08(0x56, 1, 0xD);
    }
}

int func_8019E620(void) {
    unsigned int ready = 1;
    unsigned int p1_slot = func_8018E69C(0);
    unsigned int p2_slot = func_8018E69C(1);
    unsigned int p2_not_two = ((p2_slot & 0xFF) != 2) ? 1 : 0;
    unsigned int state = D_8010A322;
    int i;
    unsigned int input;

    if (state >= 0x21) {
        func_801A63A8();
        return 0;
    }

    switch (state) {
    case 0:
        /* Initialize team-select state, 3D flag cloth meshes, and unlock flags */
        D_801DAE50 = 0;
        D_801DA064 = 0;
        D_801DA060 = 0;
        D_801DA05C = 0;
        D_801DA058 = 0;
        B(Scratchpad, 0x1EC) = 0;
        func_80096050();
        func_8005C49C();

        /* Configure 3D flag cloth mesh parameters (D_8010A370..D_8010A384) */
        S(D_800FF748, 0xAC28) = -0x69;
        S(D_800FF748, 0xAC2A) = 0x69;
        S(D_800FF748, 0xAC2E) = -0x1E;
        S(D_800FF748, 0xAC2C) = -0x1E;
        S(D_800FF748, 0xAC32) = 0x300;
        S(D_800FF748, 0xAC30) = 0x300;
        S(D_800FF748, 0xAC3C) = -0x20;
        S(D_800FF748, 0xAC34) = -0x20;
        B(D_800FF748, 0xAC21) = 0;
        B(D_800FF748, 0xAC20) = 0;

        B(Scratchpad, 0x2A2) = 1;
        if (B(Scratchpad, 0x2E1) == 5) {
            D_801D932F = 0;
            B(D_800FF748, 0x5FA4) = 0;
        }

        /* Clear 44 team availability entries (D_801DAD88[0..43]) */
        D_801D1F60 = 0;
        D_801D1F61 = 0;
        for (i = 0x2B; i >= 0; i--) {
            D_801DAD88[i] = 0;
        }

        /* Lock special teams 40..43 unless corresponding unlock bits in +0xABE2 are set */
        if ((B(D_800FF748, 0xABE2) & 1) != 0 && B(Scratchpad, 0x2E1) == 0) {
            D_801DADB3 = 0;
        } else {
            D_801DADB3 = 3;
        }
        if ((B(D_800FF748, 0xABE2) & 2) != 0) {
            D_801DADB0 = 0;
            D_801DADB1 = 0;
        } else {
            D_801DADB0 = 3;
            D_801DADB1 = 3;
        }
        if ((B(D_800FF748, 0xABE2) & 4) != 0) {
            D_801DADB2 = 0;
        } else {
            D_801DADB2 = 3;
        }

        /* Reset out-of-range or locked team IDs to defaults (0 and 10) */
        if (B(Scratchpad, 0x2DD) >= 0x2C || D_801DAD88[B(Scratchpad, 0x2DD)] == 3) {
            B(Scratchpad, 0x2DD) = 0;
        }
        if (B(Scratchpad, 0x2DE) >= 0x2C || D_801DAD88[B(Scratchpad, 0x2DE)] == 3) {
            B(Scratchpad, 0x2DE) = 0xA;
        }

        D_801D803B = 0;
        D_801D8B17 = 1;
        if (B(Scratchpad, 0x2E1) == 3) {
            B(Scratchpad, 0x2DD) = 0x29;
            B(Scratchpad, 0x2DE) = 0x28;
            D_801D819E = 0;
            D_801D819F = 0;
        }

        func_801A3588();
        func_801A2610();
        D_8010A322 = (unsigned char)(D_8010A322 + 1);
        break;

    case 1:
        /* Wait for screen/fade-in completion */
        ready &= func_801A29FC();
        if (B(Scratchpad, 0x2E1) == 5) {
            ready &= func_8018E994(D_800FF748 + 0x5E08, 0x1000, 0x100);
            ready &= func_8018E994(D_800FF748 + 0x5E30, 0x1000, 0x100);
        }
        if (ready != 0) {
            D_8010A322 = (unsigned char)(D_8010A322 + 1);
        }
        break;

    case 2:
        /* Spawn initial flag/player-label sprites and transition to state 10 */
        if (B(Scratchpad, 0x2E1) == 3) {
            B(D_800FF748, 0xAC20) = 1;
            B(D_800FF748, 0xAC21) = 0;
            func_800501DC(0);
            func_800501DC(1);
            func_80193FB4(0x12, 0x3130, 0, 0, 0, 0x1B, 0, 0x1000, 0x1000, 0, 0, 0x80, 0x80, 0x80, 0, 2, 0);
            D_801D803B = 1;
            func_801A5764(p1_slot & 0xFF, p2_slot & 0xFF);
            D_801D819A = (unsigned char)p1_slot;
            D_801D819B = (unsigned char)p2_slot;
            func_801A62EC();
            D_801D819E = 1;
        } else {
            unsigned char team0 = B(Scratchpad, 0x2DD);
            unsigned char team1 = B(Scratchpad, 0x2DE);

            D_801D803B = 1;
            func_80193FB4(
                3, (p1_slot & 0xFF) + 0x3067, 0,
                (int)((team0 % 11) * 31), (int)((team0 / 11) * 22),
                0x1A, 0, 0x1000, 0x1000, 0, 0, 0x80, 0x80, 0x80, 0, 2, 1);
            func_80193FB4(
                4, (p2_slot & 0xFF) + 0x3067, 0,
                (int)((team1 % 11) * 31), (int)((team1 / 11) * 22),
                0x1A, 0, 0x1000, 0x1000, 0, 0, 0x80, 0x80, 0x80, 0, 3, (int)p2_not_two);
        }
        D_801DA05C = 0;
        D_801DA058 = 0;
        D_8010A322 = 0xA;
        break;

    case 10:
        /* Main selection state: P1 selection (sequential) or simultaneous 2P selection */
        update_mirror_match_palette(1);
        cycle_menu_cluts();

        if (B(Scratchpad, 0x2E2) == 1) {
            /* Simultaneous 2P selection */
            func_801A2CA0(1, 1);
            func_801A2F90(1, 1);
            func_801A30F4(1, 1);
            func_801A346C(1, 1);
            D_801D1F60 = 1;
            D_801D1F61 = 1;

            if (B(Scratchpad, 0x2E1) == 1) {
                D_801D8199 = 1;
                B(D_800FF748, 0xAC21) = 1;
                D_801D819F = 1;
                if (B(Scratchpad, 0x2DD) == 0x28) {
                    func_801A61FC(0, 0, 0x2F);
                    D_801D81A0 = 0x45;
                } else {
                    func_801A61FC(0, 0, 0);
                    D_801D81A0 = 0x16;
                }
            } else {
                set_team_flag_uv(0x5E48, B(Scratchpad, 0x2DD));
            }

            /* P1 input handling */
            if (D_801DA058 == 0) {
                input = func_801A39E4(0, 0, 0, 0);
                if (input == 0x20) {
                    if (B(Scratchpad, 0x2E1) == 3) {
                        D_801D8199 = 1;
                        D_801D819C = 1;
                    } else {
                        D_801DAD88[B(Scratchpad, 0x2DD)] = 1;
                        func_8006FAA8(3, (p1_slot & 0xFF) + 0x306E);
                    }
                    func_800AB620(0x528);
                    D_801DA058++;
                } else if (input == 0x40) {
                    D_801D8199 = 0;
                    D_801D819C = 0;
                    D_801D819F = 0;
                    D_801DAE50 = 1;
                    func_800AB620(0x529);
                    D_8010A322 = 0x1E;
                }
            } else {
                if (func_801A39E4(0, 0, 1, 0) == 0x40) {
                    if (B(Scratchpad, 0x2E1) == 3) {
                        D_801D819C = 0;
                    } else {
                        D_801DAD88[B(Scratchpad, 0x2DD)] = 0;
                        func_8006FAA8(3, (p1_slot & 0xFF) + 0x3067);
                    }
                    func_800AB620(0x529);
                    D_801DA058--;
                }
            }

            /* P2 display & input handling */
            if (B(Scratchpad, 0x2E1) == 3) {
                if (B(Scratchpad, 0x2DD) == 0x28) {
                    func_801A61FC(1, 0, 0);
                    D_801D81A1 = 0x16;
                    B(Scratchpad, 0x2DE) = 0x29;
                } else {
                    func_801A61FC(1, 0, 0x2F);
                    D_801D81A1 = 0x45;
                    B(Scratchpad, 0x2DE) = 0x28;
                }
            } else {
                set_team_flag_uv(0x5E70, B(Scratchpad, 0x2DE));
            }

            if (D_801DA05C == 0) {
                if (func_801A39E4(1, 1, 0, 0) == 0x20) {
                    if (B(Scratchpad, 0x2E1) == 3) {
                        D_801D819D = 1;
                    } else {
                        D_801DAD88[B(Scratchpad, 0x2DE)] = 1;
                        func_8006FAA8(4, (p2_slot & 0xFF) + 0x306E);
                    }
                    func_800AB620(0x528);
                    D_801DA05C++;
                }
            } else {
                if (func_801A39E4(1, 1, 1, 0) == 0x40) {
                    if (B(Scratchpad, 0x2E1) == 3) {
                        D_801D819D = 0;
                    } else {
                        D_801DAD88[B(Scratchpad, 0x2DE)] = 0;
                        func_8006FAA8(4, (p2_slot & 0xFF) + 0x3067);
                    }
                    func_800AB620(0x529);
                    D_801DA05C--;
                }
            }

            if (D_801DA058 != 0 && D_801DA05C != 0) {
                D_801DAE50 = 2;
                D_8010A322 = 0x1E;
            }
        } else {
            /* Sequential selection: P1 chooses first */
            func_801A2CA0(1, 0);
            func_801A2F90(1, 0);
            func_801A30F4(1, 0);
            func_801A346C(1, (B(Scratchpad, 0x2E1) != 5) ? 1 : 0);
            D_801D1F60 = 1;
            D_801D1F61 = 0;

            if (B(Scratchpad, 0x2E1) == 3) {
                if (B(Scratchpad, 0x2DD) == 0x28) {
                    func_801A61FC(0, 0, 0x2F);
                    D_801D81A0 = 0x45;
                } else {
                    func_801A61FC(0, 0, 0);
                    D_801D81A0 = 0x16;
                }
            } else {
                set_team_flag_uv(0x5E48, B(Scratchpad, 0x2DD));
            }

            input = func_801A39E4(0, 0, 0, 0);
            if (input == 0x20) {
                D_801D8199 = 1;
                D_801D819C = 1;
                D_801D819F = 1;
                func_800AB620(0x528);

                if (B(Scratchpad, 0x2E1) == 3) {
                    B(D_800FF748, 0xAC21) = 1;
                    B(Scratchpad, 0x2DE) = (B(Scratchpad, 0x2DD) == 0x28) ? 0x29 : 0x28;
                    if (B(Scratchpad, 0x2DE) == 0x28) {
                        func_801A61FC(1, 0, 0x2F);
                        D_801D81A1 = 0x45;
                    } else {
                        func_801A61FC(1, 0, 0);
                        D_801D81A1 = 0x16;
                    }
                    D_8010A322 = 0x14;
                } else {
                    set_team_flag_uv(0x5E70, B(Scratchpad, 0x2DE));
                    D_801DAD88[B(Scratchpad, 0x2DD)] = 1;
                    func_8006FAA8(3, (p1_slot & 0xFF) + 0x306E);
                    if (B(Scratchpad, 0x2E1) == 5) {
                        D_801DAE50 = 2;
                        D_8010A322 = 0x1E;
                    } else {
                        B(D_800FF748, 0x5E64) = 1;
                        D_8010A322 = 0x14;
                    }
                }
            } else if (input == 0x40) {
                D_801D8199 = 0;
                D_801D819C = 0;
                D_801D819F = 0;
                func_800AB620(0x529);
                D_801DAE50 = 1;
                D_8010A322 = 0x1E;
            }
        }
        break;

    case 20:
        /* Sequential selection: P2 / away team selection */
        update_mirror_match_palette(0);
        cycle_menu_cluts();

        func_801A2CA0(1, 1);
        func_801A2F90(1, 1);
        func_801A30F4(1, 1);
        func_801A346C(1, 1);
        D_801D1F60 = 1;
        D_801D1F61 = 1;

        if (B(Scratchpad, 0x2E1) == 3) {
            if (B(Scratchpad, 0x2DE) == 0x28) {
                func_801A61FC(1, 0, 0x2E);
                D_801D81A1 = 0x45;
            } else {
                func_801A61FC(1, 0, 0);
                D_801D81A1 = 0x16;
            }
        } else {
            set_team_flag_uv(0x5E70, B(Scratchpad, 0x2DE));
        }

        input = func_801A39E4(1, 0, 0, 0);
        if (input == 0x20) {
            D_801D8199 = 1;
            D_801D819D = 1;
            D_801DAD88[B(Scratchpad, 0x2DE)] = 1;
            func_8006FAA8(4, (p2_slot & 0xFF) + 0x306E);
            func_800AB620(0x528);
            D_801DAE50 = 2;
            D_8010A322 = 0x1E;
        } else if (input == 0x40) {
            D_801D8199 = 0;
            D_801D819C = 0;
            B(D_800FF748, 0xAC21) = 0;
            D_801D819F = 0;
            D_801DAD88[B(Scratchpad, 0x2DD)] = 0;
            func_8006FAA8(3, (p1_slot & 0xFF) + 0x3067);
            func_800AB620(0x529);
            func_8006FA84(4, 0);
            D_8010A322 = 0xA;
        }
        break;

    case 30:
        /* Teardown / cleanup before exit fade */
        B(D_800FF748, 0x5E3C) = 0;
        B(D_800FF748, 0x5E64) = 0;
        D_801D8FE8 = 0;
        D_801D9000 = 0;
        D_801D9018 = 0;
        D_801D803B = 0;

        if (B(Scratchpad, 0x2E1) == 3) {
            func_80193FB4(0x12, 0x3130, 0, 0, 0, 0x1B, 0, 0x1000, 0x1000, 0, 0, 0x80, 0x80, 0x80, 0, 2, 0);
            D_801D8198 = 0;
            D_801D8199 = 0;
            D_801D819C = 0;
            D_801D819D = 0;
        }

        func_801A30F4(0, 0);
        func_801A346C(0, 0);
        D_801D1F60 = 0;
        D_801D1F61 = 0;
        B(D_800FF748, 0xAC21) = 0;
        B(D_800FF748, 0xAC20) = 0;
        D_801D819E = 0;
        D_801D819F = 0;
        D_8010A322 = (unsigned char)(D_8010A322 + D_801DAE50);
        break;

    case 31:
        /* Exit via cancel/back */
        ready &= func_801A2B3C();
        if (B(Scratchpad, 0x2E1) == 3) {
            B(Scratchpad, 0x2DD) = 0;
            B(Scratchpad, 0x2DE) = 0xA;
        }
        if (B(Scratchpad, 0x2E1) == 5) {
            ready &= func_8018E994(D_800FF748 + 0x5E08, 0, 0x100);
            ready &= func_8018E994(D_800FF748 + 0x5E30, 0, 0x100);
        }
        if (ready != 0) {
            B(Scratchpad, 0x2A2) = 0;
            if (B(Scratchpad, 0x2E1) == 5) {
                func_8018E720(0, B(Scratchpad, 0x2E2) + 0x1D);
            } else {
                func_8018E720(0, B(Scratchpad, 0x2E1) + 1);
            }
            func_8018E4AC(3, 0xF, 0);
            return 1;
        }
        break;

    case 32:
        /* Exit via confirmation */
        if (func_801A2B3C() != 0) {
            B(Scratchpad, 0x2A2) = 0;
            func_8018E4AC(3, 0xF, 0);
            return 2;
        }
        break;

    default:
        break;
    }

    func_801A63A8();
    return 0;
}
