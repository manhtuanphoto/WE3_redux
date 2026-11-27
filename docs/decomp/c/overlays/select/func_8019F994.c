/*
 * func_8019F994 (select overlay)
 *
 * Purpose:
 *   Main state machine for the team selection screen (supporting both 2-team
 *   exhibition/single-match mode when (D_800FF748[0x8F1F] & 0xF) == 0 and
 *   multi-team cup/league slot selection when != 0). Dispatches across 72 state
 *   indices (0..0x47) via jump table jtbl_8018A0A0, managing slot initialization,
 *   flag-grid cursor rendering (11 columns x 31px x 22px), P1/P2/slot selection,
 *   slot swapping, team removal/clearing, random/CPU auto-selection, and
 *   confirm/cancel transitions.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   int:
 *     0 = remain in team select state machine
 *     1 = cancel / exit back (restoring Scratchpad+0x2DE from D_801D8038)
 *     2 = confirm / proceed to next screen (when func_8006F0DC(0x10, 1) != 0)
 *
 * Global fields read / written:
 *   D_8010A322 (u8): state machine index (0..0x47)
 *   D_8010A5A4 (u16): secondary input/button state (checked for 0x40 cancel)
 *   D_8011BD54 (u8[]): default team ID lookup table by slot index
 *   D_800FF748 (u8[]): shared state/actor buffer:
 *     +0x5E3C (u8)  cursor/highlight enable flag
 *     +0x5E48 (s16) P1 flag-grid X = (team % 11) * 31
 *     +0x5E4A (s16) P1 flag-grid Y = (team / 11) * 22
 *     +0x5E64 (u8)  P2/secondary cursor blink flag
 *     +0x5E70 (s16) P2 flag-grid X = (team % 11) * 31
 *     +0x5E72 (s16) P2 flag-grid Y = (team / 11) * 22
 *     +0x606C (u8)  cleared on exit
 *     +0x6074 (u32) pointer/handle from func_8001D004(0x3073 + phase)
 *     +0x8F1F (u8)  competition/match flags (low nibble & 0xF: 0 = exhibition, != 0 = cup/league)
 *     +0x8F20 (u8)  target number of team slots to fill
 *     +0x8F24 + slot*36 (u8) slot flags/controller assignment (0xFF = empty, & 0xC0 == 0xC0 = empty)
 *     +0x8F25 + slot*36 (u8) assigned team ID in slot (0xFF = empty)
 *     +0xAB80[32] (u8) slot index permutation/mapping table
 *     +0xABDA (u8)  overlay sub-state/phase value (0x0A, 0x14, 0x1E, 0x28, 0x32, 0x3C, 0x46, etc.)
 *     +0xAC20..+0xAC3C: layout/camera/window geometry parameters
 *   D_801D1F60, D_801D1F61 (u8): UI layer active flags
 *   D_801D8038 (u8): saved initial Scratchpad+0x2DE
 *   D_801D803B (u8): UI state flag
 *   D_801D803C (u8): saved/source slot index for swap/edit
 *   D_801D8A70 (u8): current slot cursor index
 *   D_801D8A82..D_801D8A88 (s16): slot cursor box coordinates (x1, y1, x2, y2)
 *   D_801D8A8A..D_801D8A8C (u8): slot cursor RGB color
 *   D_801D8A8D (u8): slot cursor alpha/parameter (0x60)
 *   D_801D8A8E (u8): slot cursor flags
 *   D_801D8A8F (u8): slot cursor visible flag
 *   D_801D8B17 (u8): selection focus (0 = slot list, 1 = team flag grid)
 *   D_801D8C6C (u32): polled pad/button state from func_801A3FDC()
 *   D_801D8DC0[44] (u8): per-team locked/used flags (cleared for 0..43)
 *   D_801DAD88[44] (u8): per-team user-picked flags
 *   D_801DA058 (s32): banner toggle frame counter (mod 400 / 200)
 *   D_801DA05C (s32): count of currently selected teams
 *   D_801DA060, D_801DA064 (u32): animation/scroll timers
 *   D_801DA331 (u8): active slot controller/side attribute from func_801A49D4()
 *   D_801DAE50 (u8): completion/transition state (0 = init, 2 = complete)
 *
 * Scratchpad (0x1F800000) fields read / written:
 *   +0x1EC (u8): cleared to 0 on init
 *   +0x290 (u32): global frame counter (bit 0x10 used for cursor blink)
 *   +0x2A2 (u8): overlay active mode (set to 2 on init, 0 on exit)
 *   +0x2DD (u8): primary/active cursor team ID
 *   +0x2DE (u8): secondary/away cursor team ID
 *   +0x2E1 (u8): game mode (checked == 1 on cancel)
 */

#define B(p, o)   (*(volatile unsigned char *)((p) + (o)))
#define H(p, o)   (*(volatile unsigned short *)((p) + (o)))
#define SH(p, o)  (*(volatile short *)((p) + (o)))
#define W(p, o)   (*(volatile unsigned int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_8010A322;
extern unsigned short D_8010A5A4;
extern unsigned char D_8011BD54[];
extern unsigned char D_800FF748[];

extern unsigned char D_801D1F60;
extern unsigned char D_801D1F61;
extern unsigned char D_801D8038;
extern unsigned char D_801D803B;
extern unsigned char D_801D803C;
extern unsigned char D_801D8A70;
extern short D_801D8A82;
extern short D_801D8A84;
extern short D_801D8A86;
extern short D_801D8A88;
extern unsigned char D_801D8A8A;
extern unsigned char D_801D8A8B;
extern unsigned char D_801D8A8C;
extern unsigned char D_801D8A8D;
extern unsigned char D_801D8A8E;
extern unsigned char D_801D8A8F;
extern unsigned char D_801D8B17;
extern unsigned int D_801D8C6C;
extern unsigned char D_801D8DC0[];
extern unsigned char D_801DAD88[];
extern int D_801DA058;
extern int D_801DA05C;
extern unsigned int D_801DA060;
extern unsigned int D_801DA064;
extern unsigned char D_801DA331;
extern unsigned char D_801DAE50;

extern unsigned int func_8001D004(unsigned int texId);
extern int func_8001E6B8(int range);
extern void func_8005C49C(void);
extern int func_8006F0DC(int a0, int a1);
extern void func_8006FAA8(int a0, int a1);
extern void func_8006FC08(int a0, int a1, int a2);
extern void func_80096050(void);
extern void func_800AB620(unsigned int sfx);
extern int func_8018E4AC(int a0, int a1, int a2);
extern int func_8018E5B8(int a0, int a1);
extern void func_8018E720(int a0, int a1);
extern void func_80193FB4(
    int a0, int a1, int a2, int a3,
    int s0, int s1, int s2, int s3,
    int s4, int s5, int s6, int s7,
    int s8, int s9, int s10, int s11,
    int s12
);
extern void func_801A2198(unsigned int teamId);
extern void func_801A24E4(void);
extern void func_801A2610(void);
extern int func_801A29FC(void);
extern int func_801A2B3C(void);
extern void func_801A2CA0(int isCupMode, int a1);
extern void func_801A2F90(int a0, int a1);
extern void func_801A30F4(int isCupMode, int a1);
extern void func_801A346C(int a0, int a1);
extern void func_801A3588(void);
extern void func_801A372C(int modeLowNibble);
extern void func_801A3930(void);
extern unsigned int func_801A3FDC(void);
extern unsigned char func_801A49D4(int param);
extern void func_801A4C6C(void);
extern int func_801AB6E8(int dialogId);

static void update_exhibition_banner(void)
{
    int frame = (D_801DA058 + 1) % 400;
    D_801DA058 = frame;
    if ((frame % 200) == 0) {
        W(D_800FF748, 0x6074) = func_8001D004((unsigned int)((frame / 200) + 0x3073));
    }
}

static void update_team_select_ui_common(void)
{
    int isCup = (B(D_800FF748, 0x8F1F) & 0x0F) != 0;

    func_801A2CA0(isCup, 0);
    func_801A2F90(1, 0);
    func_801A30F4(isCup, 0);
    func_801A346C(0, 0);
    D_801D1F60 = 1;
    D_801D1F61 = 0;
    func_801A3588();
}

static void set_cursor_color_from_attr(unsigned char attr)
{
    if ((attr & 0xC0) == 0) {
        D_801D8A8A = 0x10;
        D_801D8A8B = 0x60;
        D_801D8A8C = 0x80;
    } else {
        D_801D8A8A = 0x80;
        D_801D8A8B = 0x80;
        D_801D8A8C = 0x00;
    }
}

static void swap_slot_entries(unsigned char slotIdxA, unsigned char slotIdxB)
{
    unsigned char entryA = B(D_800FF748, 0xAB80 + slotIdxA);
    unsigned char entryB = B(D_800FF748, 0xAB80 + slotIdxB);
    unsigned char tmpFlags = B(D_800FF748, 0x8F24 + entryA * 36);
    unsigned char tmpTeam;

    B(D_800FF748, 0x8F24 + entryA * 36) = B(D_800FF748, 0x8F24 + entryB * 36);
    B(D_800FF748, 0x8F24 + entryB * 36) = tmpFlags;

    entryA = B(D_800FF748, 0xAB80 + slotIdxA);
    entryB = B(D_800FF748, 0xAB80 + slotIdxB);
    tmpTeam = B(D_800FF748, 0x8F25 + entryA * 36);
    B(D_800FF748, 0x8F25 + entryA * 36) = B(D_800FF748, 0x8F25 + entryB * 36);
    B(D_800FF748, 0x8F25 + entryB * 36) = tmpTeam;
}

int func_8019F994(void)
{
    unsigned int state = D_8010A322;
    int i;

    if (state >= 0x48) {
        return 0;
    }

    switch (state) {
    case 0:
        /* Initial setup: clear selection counts, initialize 32 slot entries, reset geometry */
        func_801A2CA0(0, 0);
        D_801DAE50 = 0;
        D_801D8A70 = 0;
        D_801D8B17 = 1;
        D_801DA064 = 0;
        D_801DA060 = 0;
        D_801DA05C = 0;
        D_801DA058 = 0;

        D_801D8038 = B(Scratchpad, 0x2DE);
        B(Scratchpad, 0x2DE) = 0xFF;

        func_801A3588();
        for (i = 0; i < 32; i++) {
            unsigned char entry;
            B(D_800FF748, 0xAB80 + i) = (unsigned char)i;
            entry = B(D_800FF748, 0xAB80 + i);
            B(D_800FF748, 0x8F24 + entry * 36) = 0xFF;
            entry = B(D_800FF748, 0xAB80 + i);
            B(D_800FF748, 0x8F25 + entry * 36) = 0xFF;
        }

        func_801A2610();
        B(Scratchpad, 0x2A2) = 2;
        D_801D803B = 0;
        D_801D803C = 0;
        B(Scratchpad, 0x1EC) = 0;
        func_80096050();
        func_8005C49C();

        SH(D_800FF748, 0xAC28) = -105;
        SH(D_800FF748, 0xAC2A) = 105;
        SH(D_800FF748, 0xAC2E) = -30;
        SH(D_800FF748, 0xAC2C) = -30;
        SH(D_800FF748, 0xAC32) = 0x300;
        SH(D_800FF748, 0xAC30) = 0x300;
        SH(D_800FF748, 0xAC3C) = -32;
        SH(D_800FF748, 0xAC34) = -32;
        B(D_800FF748, 0xAC21) = 0;
        B(D_800FF748, 0xAC20) = 0;

        D_801D8A8C = 0x80;
        D_801D8A82 = 0;
        D_801D8A84 = 0;
        D_801D8A86 = 0;
        D_801D8A88 = 0;
        D_801D8A8F = 0;
        D_801D8A8A = 0;
        D_801D8A8B = 0;
        D_801D8A8D = 0x60;
        D_801D8A8E &= 0x40;
        B(D_800FF748, 0xABDA)++;
        return 0;

    case 1:
        /* Wait until fade/transition helper func_801A29FC() reports ready */
        if (func_801A29FC() != 0) {
            goto L801A20D0;
        }
        return 0;

    case 2:
        /* Initialize flag grid sprites/primitives, clear team lock table, transition to 0x0A */
        {
            unsigned int team = B(Scratchpad, 0x2DD);
            int colX = (int)(team % 11U) * 31;
            int rowY = (int)(team / 11U) * 22;

            /* UNCLEAR: Exact parameter semantics of func_80193FB4 (17 arguments) */
            func_80193FB4(3, 0x306C, 0, colX, rowY, 0x1A, 0, 0x1000, 0x1000, 0, 0, 0x80, 0x80, 0x80, 0, 2, 1);

            team = B(Scratchpad, 0x2DD);
            colX = (int)(team % 11U) * 31;
            rowY = (int)(team / 11U) * 22;
            func_80193FB4(4, 0x306C, 0, colX, rowY, 0x1A, 0, 0x1000, 0x1000, 0, 0, 0x80, 0x80, 0x80, 0, 2, 0);
        }

        for (i = 0x2B; i >= 0; i--) {
            D_801D8DC0[i] = 0;
        }

        if ((B(D_800FF748, 0x8F1F) & 0x0F) == 0) {
            func_80193FB4(0x11, 0x3074, 0, 0, 0, 0x1C, 0, 0x1000, 0x1000, 0, 0, 0x80, 0x80, 0x80, 0, 2, 1);
            D_801DA058 = 0x190;
        } else {
            func_80193FB4(0x11, 0x3073, 0, 0, 0, 0x1C, 0, 0x1000, 0x1000, 0, 0, 0x80, 0x80, 0x80, 0, 2, 1);
        }

        func_801A2F90(1, 0);
        func_801A30F4((B(D_800FF748, 0x8F1F) & 0x0F) != 0, 0);
        func_801A346C(1, 0);
        D_801D1F60 = 1;
        D_801D1F61 = 0;
        D_801D8A8F = 1;
        func_801A4C6C();
        D_801D803B = 1;
        D_801DA331 = 0;
        B(D_800FF748, 0xABDA) = 0x0A;
        return 0;

    case 10:
        /* Main interactive selection state (0x0A) */
        update_team_select_ui_common();
        D_801D8A8E &= 0xF0;
        func_801A4C6C();

        if ((B(D_800FF748, 0x8F1F) & 0x0F) == 0) {
            update_exhibition_banner();
        }

        {
            unsigned int team = B(Scratchpad, 0x2DD);
            B(D_800FF748, 0x5E64) = 0;
            B(D_800FF748, 0x5E3C) = (D_801D8B17 != 0) ? 1 : 0;
            SH(D_800FF748, 0x5E48) = (short)((team % 11U) * 31);
            SH(D_800FF748, 0x5E4A) = (short)((team / 11U) * 22);
        }

        D_801DA331 = func_801A49D4(0);
        if ((D_801DA331 & 0xC0) == 0) {
            func_8006FAA8(3, 0x306C);
            func_8006FC08(0x54, 1, 0x0D);
        } else {
            func_8006FAA8(3, 0x306D);
            func_8006FC08(0x56, 1, 0x0D);
        }
        set_cursor_color_from_attr(D_801DA331);

        D_801D8C6C = func_801A3FDC();
        if (D_801D8C6C == 0x20) {
            unsigned int mode = D_801D8B17;
            if (mode == 0) {
                unsigned char entry = B(D_800FF748, 0xAB80 + D_801D8A70);
                if ((B(D_800FF748, 0x8F24 + entry * 36) & 0xC0) != 0xC0) {
                    func_800AB620(0x527);
                    D_801D803C = D_801D8A70;
                    entry = B(D_800FF748, 0xAB80 + D_801D8A70);
                    B(Scratchpad, 0x2DE) = B(D_800FF748, 0x8F25 + entry * 36);
                    B(D_800FF748, 0xABDA) = 0x28;
                }
            } else if (mode == 1) {
                if (D_801DA05C < (int)B(D_800FF748, 0x8F20)) {
                    unsigned int team = B(Scratchpad, 0x2DD);
                    if (D_801D8DC0[team] == 1 || D_801DAD88[team] != 0) {
                        func_800AB620(0x512);
                    } else {
                        func_800AB620(0x528);
                        B(Scratchpad, 0x2DE) = (unsigned char)team;
                        func_801A2198(team);
                        D_801D8B17 = 0;
                        B(D_800FF748, 0xABDA) = 0x14;
                    }
                } else {
                    func_800AB620(0x528);
                    D_801DAE50 = 2;
                    B(D_800FF748, 0xABDA) = 0x32;
                }
            }
        }

        if (D_801D8C6C == 0x40) {
            if (D_801D8B17 == 0) {
                if (D_801DA05C == 0) {
                    func_800AB620(0x529);
                    B(D_800FF748, 0xABDA) = 0x46;
                } else {
                    unsigned char entry = B(D_800FF748, 0xAB80 + D_801D8A70);
                    if ((B(D_800FF748, 0x8F24 + entry * 36) & 0xC0) == 0xC0) {
                        if (B(Scratchpad, 0x2DE) != 0xFF) {
                            func_800AB620(0x529);
                            B(Scratchpad, 0x2DD) = B(Scratchpad, 0x2DE);
                            D_801D8B17 = 1;
                        }
                    } else {
                        unsigned char removedTeam;
                        func_800AB620(0x529);
                        entry = B(D_800FF748, 0xAB80 + D_801D8A70);
                        removedTeam = B(D_800FF748, 0x8F25 + entry * 36);
                        D_801D8DC0[removedTeam] = 0;
                        D_801DAD88[removedTeam] = 0;
                        entry = B(D_800FF748, 0xAB80 + D_801D8A70);
                        B(D_800FF748, 0x8F24 + entry * 36) = 0xFF;
                        entry = B(D_800FF748, 0xAB80 + D_801D8A70);
                        B(D_800FF748, 0x8F25 + entry * 36) = 0xFF;
                        B(Scratchpad, 0x2DD) = removedTeam;
                        D_801DA05C--;
                        D_801DA331 = func_801A49D4(removedTeam);
                        D_801D8B17 = 1;
                    }
                }
            } else if (D_801D8B17 == 1) {
                if (D_801DA05C == 0) {
                    func_800AB620(0x529);
                    B(D_800FF748, 0xABDA) = 0x46;
                } else if (D_801DAD88[B(Scratchpad, 0x2DD)] == 1) {
                    int targetCount;
                    func_800AB620(0x529);
                    D_801DA05C--;
                    D_801DAD88[B(Scratchpad, 0x2DD)] = 0;
                    D_801D8DC0[B(Scratchpad, 0x2DD)] = 0;
                    targetCount = B(D_800FF748, 0x8F20);
                    for (i = 0; i < targetCount; i++) {
                        unsigned char entry = B(D_800FF748, 0xAB80 + i);
                        if (B(D_800FF748, 0x8F25 + entry * 36) == B(Scratchpad, 0x2DD)) {
                            unsigned char flags = B(D_800FF748, 0x8F24 + entry * 36);
                            B(D_800FF748, 0x8F24 + entry * 36) = 0xFF;
                            B(D_800FF748, 0x8F25 + B(D_800FF748, 0xAB80 + i) * 36) = 0xFF;
                            D_801D8A70 = (unsigned char)i;
                            D_801DA331 = func_801A49D4(flags & 0xC0);
                            break;
                        }
                    }
                }
            }
        }

        if (D_801D8C6C == 0x10 || ((B(D_800FF748, 0x8F1F) & 0x0F) == 0 && D_801D8C6C == 0x80)) {
            D_801D8B17 = 1;
            if (D_801DA05C == (int)B(D_800FF748, 0x8F20)) {
                func_800AB620(0x528);
                goto L801A1740_finish;
            }
            B(D_800FF748, 0xABDA) = 0x1E;
        }
        return 0;

    case 20:
        /* Secondary selection / slot assignment state */
        update_team_select_ui_common();
        D_801D8A8E &= 0xF0;
        func_801A4C6C();

        if ((B(D_800FF748, 0x8F1F) & 0x0F) == 0) {
            update_exhibition_banner();
        }

        {
            unsigned int team = B(Scratchpad, 0x2DE);
            B(D_800FF748, 0x5E3C) = 0;
            B(D_800FF748, 0x5E64) = (unsigned char)(W(Scratchpad, 0x290) & 0x10);
            SH(D_800FF748, 0x5E70) = (short)((team % 11U) * 31);
            SH(D_800FF748, 0x5E72) = (short)((team / 11U) * 22);
        }

        if ((D_801DA331 & 0xC0) == 0) {
            func_8006FC08(0x54, 1, 0x0D);
        } else {
            func_8006FC08(0x56, 1, 0x0D);
        }
        set_cursor_color_from_attr(D_801DA331);

        D_801D8C6C = func_801A3FDC();
        if (D_801D8C6C == 0x20) {
            if (D_801D8B17 == 0) {
                if (D_801D8A70 < B(D_800FF748, 0x8F20)) {
                    unsigned char entry;
                    func_800AB620(0x528);
                    D_801DA05C++;
                    entry = B(D_800FF748, 0xAB80 + D_801D8A70);
                    if ((B(D_800FF748, 0x8F24 + entry * 36) & 0xC0) != 0xC0) {
                        unsigned char prevTeam = B(D_800FF748, 0x8F25 + entry * 36);
                        D_801D8DC0[prevTeam] = 0;
                        D_801DAD88[prevTeam] = 0;
                        D_801DA05C--;
                    }
                    entry = B(D_800FF748, 0xAB80 + D_801D8A70);
                    B(D_800FF748, 0x8F24 + entry * 36) = D_801DA331;
                    entry = B(D_800FF748, 0xAB80 + D_801D8A70);
                    B(D_800FF748, 0x8F25 + entry * 36) = B(Scratchpad, 0x2DE);
                    D_801DAD88[B(Scratchpad, 0x2DE)] = 1;
                    D_801D8DC0[B(Scratchpad, 0x2DE)] = 1;
                    D_801D8B17 = 1;

                    if (D_801DA05C == (int)B(D_800FF748, 0x8F20)) {
                        if ((unsigned int)D_801D8A70 >= (unsigned int)D_801DA05C) {
                            D_801D8A70--;
                        }
                        D_801DAE50 = 2;
                        B(D_800FF748, 0xABDA) = 0x32;
                        func_801A3588();
                        return 0;
                    }
                    B(D_800FF748, 0xABDA) = 0x0A;
                } else {
                    func_800AB620(0x512);
                }
                return 0;
            } else if (D_801D8B17 == 1) {
                if (D_801DA05C < (int)B(D_800FF748, 0x8F20)) {
                    unsigned int team = B(Scratchpad, 0x2DD);
                    if (D_801D8DC0[team] == 1 || D_801DAD88[team] != 0) {
                        func_800AB620(0x512);
                    } else {
                        B(Scratchpad, 0x2DE) = (unsigned char)team;
                        func_801A2198(team);
                        D_801D8B17 = 0;
                        func_800AB620(0x528);
                    }
                }
                return 0;
            } else {
                return 0;
            }
        }

        if (D_8010A5A4 == 0x40) {
            func_800AB620(0x529);
            if (D_801D8B17 == 0) {
                if (D_801DA05C != 0) {
                    unsigned char entry;
                    func_800AB620(0x529);
                    entry = B(D_800FF748, 0xAB80 + D_801D8A70);
                    if ((B(D_800FF748, 0x8F24 + entry * 36) & 0xC0) == 0xC0) {
                        B(Scratchpad, 0x2DD) = B(Scratchpad, 0x2DE);
                    } else {
                        unsigned char removedTeam = B(D_800FF748, 0x8F25 + entry * 36);
                        D_801D8DC0[removedTeam] = 0;
                        D_801DAD88[removedTeam] = 0;
                        entry = B(D_800FF748, 0xAB80 + D_801D8A70);
                        B(D_800FF748, 0x8F24 + entry * 36) = 0xFF;
                        entry = B(D_800FF748, 0xAB80 + D_801D8A70);
                        B(D_800FF748, 0x8F25 + entry * 36) = 0xFF;
                        B(Scratchpad, 0x2DD) = removedTeam;
                        D_801DA05C--;
                        D_801DA331 = func_801A49D4(removedTeam);
                    }
                }
                D_801D8B17 = 1;
                B(D_800FF748, 0xABDA) = 0x0A;
            } else if (D_801D8B17 == 1) {
                func_800AB620(0x529);
                B(Scratchpad, 0x2DD) = B(Scratchpad, 0x2DE);
                B(D_800FF748, 0xABDA) = 0x0A;
            }
        }
        return 0;

    case 30:
        /* Auto/quick assign current Scratchpad+0x2DD team into current slot D_801D8A70 */
        update_team_select_ui_common();
        D_801DA060 = 0;
        D_801DA064 = 0;

        {
            unsigned int team = B(Scratchpad, 0x2DD);
            if (D_801D8DC0[team] == 0 && D_801DAD88[team] == 0) {
                unsigned char entry;
                func_801A2198(team);
                D_801D8A8F = 1;
                D_801DA331 = func_801A49D4(0);
                set_cursor_color_from_attr(D_801DA331);
                func_800AB620(0x528);

                D_801DA05C++;
                entry = B(D_800FF748, 0xAB80 + D_801D8A70);
                B(D_800FF748, 0x8F24 + entry * 36) = D_801DA331;
                entry = B(D_800FF748, 0xAB80 + D_801D8A70);
                B(D_800FF748, 0x8F25 + entry * 36) = B(Scratchpad, 0x2DD);
                D_801DAD88[B(Scratchpad, 0x2DD)] = 1;
                D_801D8DC0[B(Scratchpad, 0x2DD)] = 1;

                if (D_801DA05C == (int)B(D_800FF748, 0x8F20)) {
                    goto L801A1740_finish;
                }

                {
                    int isCup = (B(D_800FF748, 0x8F1F) & 0x0F) != 0;
                    func_801A2CA0(isCup, 0);
                    func_801A2F90(1, 0);
                    D_801DA064 = 8;
                    D_801DA060 = 8;
                    func_801A30F4(isCup, 0);
                    func_801A346C(0, 0);
                    D_801D1F60 = 1;
                    D_801D1F61 = 0;
                }
            }
        }

        if ((B(D_800FF748, 0x8F1F) & 0x0F) == 0 && D_801D8C6C == 0x80) {
            goto L801A20D0;
        }
        B(D_800FF748, 0xABDA) += 2;
        return 0;

    case 31:
        /* Auto-fill next empty slot from default table D_8011BD54 (exhibition / grid mode) */
        D_801D8A8F = 1;
        D_801DA331 = func_801A49D4((func_8001E6B8(2) & 0xFF) == 1 ? 0x40 : 0);

        D_801D8A70 = 0;
        if ((B(D_800FF748, 0x8F24 + B(D_800FF748, 0xAB80) * 36) & 0xC0) != 0xC0) {
            do {
                D_801D8A70++;
            } while ((B(D_800FF748, 0x8F24 + B(D_800FF748, 0xAB80 + D_801D8A70) * 36) & 0xC0) != 0xC0);
        }

        B(Scratchpad, 0x2DD) = D_8011BD54[D_801D8A70];
        while (D_801D8DC0[B(Scratchpad, 0x2DD)] == 1) {
            for (i = 0; i < 32; i++) {
                unsigned char entry = B(D_800FF748, 0xAB80 + i);
                if (B(D_800FF748, 0x8F25 + entry * 36) == B(Scratchpad, 0x2DD)) {
                    B(Scratchpad, 0x2DD) = D_8011BD54[i];
                    break;
                }
            }
        }

        update_team_select_ui_common();
        {
            unsigned int team = B(Scratchpad, 0x2DD);
            B(D_800FF748, 0x5E3C) = 1;
            SH(D_800FF748, 0x5E48) = (short)((team % 11U) * 31);
            SH(D_800FF748, 0x5E4A) = (short)((team / 11U) * 22);
        }

        if ((D_801DA331 & 0xC0) == 0) {
            func_8006FAA8(3, 0x306C);
        } else {
            func_8006FAA8(3, 0x306D);
        }
        set_cursor_color_from_attr(D_801DA331);

        D_801D8A8E |= 0x10;
        {
            unsigned int cur = D_801D8A70;
            unsigned int col = cur & 0x0F;
            short x = (short)(col * 21 + (col >> 2) * 2 - 0xA6);
            short y = (short)((cur >> 4) * 32 - 0x35);
            D_801D8A82 = x;
            D_801D8A84 = y;
            D_801D8A86 = x;
            D_801D8A88 = y;
        }
        goto L801A1650;

    case 32:
        /* Auto-fill next empty slot via func_801A24E4 (cup/bracket mode) */
        D_801DA331 = func_801A49D4(0);
        D_801D8A70 = 0;
        {
            unsigned char flags = B(D_800FF748, 0x8F24 + B(D_800FF748, 0xAB80) * 36) & 0xC0;
            while (flags != 0xC0) {
                D_801D8A70++;
                flags = B(D_800FF748, 0x8F24 + B(D_800FF748, 0xAB80 + D_801D8A70) * 36) & 0xC0;
            }
            B(Scratchpad, 0x2DD) = flags; /* 0xC0 prior to func_801A24E4() */
        }
        func_801A24E4();

        D_801DA331 = func_801A49D4((func_8001E6B8(2) & 0xFF) == 1 ? 0x40 : 0);
        update_team_select_ui_common();
        {
            unsigned int team = B(Scratchpad, 0x2DD);
            D_801DA060 = 0;
            D_801DA064 = 0;
            B(D_800FF748, 0x5E3C) = 1;
            SH(D_800FF748, 0x5E48) = (short)((team % 11U) * 31);
            SH(D_800FF748, 0x5E4A) = (short)((team / 11U) * 22);
        }

        if ((D_801DA331 & 0xC0) == 0) {
            func_8006FAA8(3, 0x306C);
        } else {
            func_8006FAA8(3, 0x306D);
        }

        D_801D8A8E |= 0x10;
        {
            unsigned int cur = D_801D8A70;
            unsigned int col = cur & 0x07;
            short x = (short)(col * 21 + 7);
            short y = (short)((cur >> 3) * 26 - 0x30);
            int fade = func_8018E5B8(0x80, 1);

            D_801D8A82 = x;
            D_801D8A84 = y;
            D_801D8A86 = x;
            D_801D8A88 = y;

            if ((D_801DA331 & 0xC0) == 0) {
                D_801D8A8B = 0x60;
                D_801D8A8A = 0x00;
                D_801D8A8C = (unsigned char)(fade + 0x80);
            } else {
                D_801D8A8A = (unsigned char)(fade + 0x80);
                D_801D8A8B = (unsigned char)(fade + 0x80);
                D_801D8A8C = 0x00;
            }
        }

    L801A1650:
        {
            unsigned char entry;
            func_800AB620(0x528);
            D_801DA05C++;
            entry = B(D_800FF748, 0xAB80 + D_801D8A70);
            B(D_800FF748, 0x8F24 + entry * 36) = D_801DA331;
            entry = B(D_800FF748, 0xAB80 + D_801D8A70);
            B(D_800FF748, 0x8F25 + entry * 36) = B(Scratchpad, 0x2DD);
            D_801DAD88[B(Scratchpad, 0x2DD)] = 1;
            D_801D8DC0[B(Scratchpad, 0x2DD)] = 1;

            if (D_801DA05C != (int)B(D_800FF748, 0x8F20)) {
                return 0;
            }
        }
    L801A1740_finish:
        D_801DAE50 = 2;
        B(D_800FF748, 0xABDA) = 0x32;
        return 0;

    case 40:
        /* Slot swap/replace target selection state (0x28) */
        update_team_select_ui_common();
        B(D_800FF748, 0x5E3C) = 0;
        B(D_800FF748, 0x5E64) = 0;
        D_801D8A8A = 0x80;
        D_801D8A8B = 0x10;
        D_801D8A8C = 0x10;
        D_801D8A8E = (D_801D8A8E & 0xF0) | 0x02;
        func_801A4C6C();

        /*
         * Note: assembly loads $v1 = 0x20 in delay slot of jal func_801A3FDC and
         * compares $v0 against $v1, relying on func_801A3FDC preserving $v1.
         */
        D_801D8C6C = func_801A3FDC();
        if (D_801D8C6C == 0x20) {
            if (D_801D8B17 == 0) {
                if (D_801D8A70 < B(D_800FF748, 0x8F20)) {
                    func_800AB620(0x527);
                    B(D_800FF748, 0xABDA)++;
                } else {
                    func_800AB620(0x512);
                }
            } else if (D_801D8B17 == 1) {
                B(D_800FF748, 0xABDA)++;
            }
        }

        if (D_801D8C6C == 0x40) {
            func_800AB620(0x529);
            B(D_800FF748, 0xABDA) = 0x0A;
        }
        return 0;

    case 41:
        /* Execute slot swap or team replacement (using source slot D_801D803C) */
        if (D_801D8B17 == 0) {
            swap_slot_entries(D_801D8A70, D_801D803C);
        } else if (D_801D8B17 == 1) {
            unsigned char picked = D_801DAD88[B(Scratchpad, 0x2DD)];
            if (picked == 1) {
                func_800AB620(0x527);
                if (D_801D8DC0[B(Scratchpad, 0x2DD)] == 1) {
                    unsigned char entry;
                    D_801D8A70 = 0;
                    entry = B(D_800FF748, 0xAB80);
                    while (B(D_800FF748, 0x8F25 + entry * 36) != B(Scratchpad, 0x2DD)) {
                        D_801D8A70++;
                        entry = B(D_800FF748, 0xAB80 + D_801D8A70);
                    }
                    swap_slot_entries(D_801D8A70, D_801D803C);
                }
            } else if (picked == 0) {
                unsigned char entry = B(D_800FF748, 0xAB80 + D_801D803C);
                unsigned char oldTeam = B(D_800FF748, 0x8F25 + entry * 36);
                func_800AB620(0x527);
                D_801D8DC0[oldTeam] = 0;
                D_801DAD88[oldTeam] = 0;
                entry = B(D_800FF748, 0xAB80 + D_801D803C);
                B(D_800FF748, 0x8F25 + entry * 36) = B(Scratchpad, 0x2DD);
                D_801DAD88[B(Scratchpad, 0x2DD)] = 1;
                D_801D8DC0[B(Scratchpad, 0x2DD)] = 1;
            } else if (picked < 4) {
                func_800AB620(0x512);
                D_801D8B17 = 0;
                D_801D8A70 = D_801D803C;
            }
        }

        D_801D803C = 0;
        B(D_800FF748, 0xABDA) = 0x0A;
        return 0;

    case 50:
        /* Hide slot cursor and advance sub-state */
        update_team_select_ui_common();
        D_801D8A8F = 0;
        B(D_800FF748, 0x5E3C) = 0;
        B(D_800FF748, 0x5E64) = 0;
        B(D_800FF748, 0xABDA)++;
        return 0;

    case 51:
        /* Wait for cup/league scroll timer (D_801DA060 >= 8) or skip ahead by 2 */
        if ((B(D_800FF748, 0x8F1F) & 0x0F) != 0) {
            func_801A30F4(1, 0);
            if (D_801DA060 < 8) {
                return 0;
            }
            goto L801A20D0;
        }
        B(D_800FF748, 0xABDA) += 2;
        return 0;

    case 52:
        /* Update UI helper and advance sub-state */
        func_801A30F4((B(D_800FF748, 0x8F1F) & 0x0F) != 0, 0);
        goto L801A20D0;

    case 53:
        /* Confirmation/reset prompt dialog (0x4D) */
        switch (func_801AB6E8(0x4D)) {
        case 1:
            /* Resume selection */
            D_801D8A8F = 1;
            D_801D8B17 = 0;
            B(D_800FF748, 0xABDA) = 0x0A;
            D_801D8A8E &= 0xEF;
            break;
        case 2:
            /* Proceed to exit sequence */
            B(D_800FF748, 0xABDA) = 0x3C;
            break;
        case 3:
            /* Clear/reset all selected teams and restart selection */
            D_801D8A8F = 1;
            D_801D8A8E &= 0xEF;
            func_801A3930();
            func_801A372C(B(D_800FF748, 0x8F1F) & 0x0F);
            D_801D8A70 = 0;
            D_801DA05C = 0;
            D_801D8B17 = 1;
            B(D_800FF748, 0xABDA) = 2;
            break;
        default:
            break;
        }
        return 0;

    case 60:
    case 70:
        /* Teardown UI panels and clear select-overlay state */
        B(D_800FF748, 0x5E3C) = 0;
        B(D_800FF748, 0x5E64) = 0;
        func_801A2F90(0, 0);
        func_801A30F4(0, 0);
        func_801A346C(0, 0);
        D_801D1F60 = 0;
        D_801D1F61 = 0;
        D_801D8A8F = 0;
        D_801D803B = 0;
        B(D_800FF748, 0x606C) = 0;
        B(D_800FF748, 0xAC21) = 0;
        B(D_800FF748, 0xAC20) = 0;
        B(D_800FF748, 0xABDA)++;
        return 0;

    case 61:
        /* Play confirm SFX (7) and advance once func_801A2B3C() completes */
        if (func_801A2B3C() == 0) {
            return 0;
        }
        func_800AB620(7);
    L801A20D0:
        B(D_800FF748, 0xABDA)++;
        return 0;

    case 62:
        /* Finalize forward transition when func_8006F0DC(0x10, 1) completes */
        if (func_8006F0DC(0x10, 1) == 0) {
            return 0;
        }
        B(Scratchpad, 0x2A2) = 0;
        return 2;

    case 71:
        /* Cancel / return to previous menu once func_801A2B3C() completes */
        if (func_801A2B3C() == 0) {
            return 0;
        }
        {
            int nextScreen = (B(Scratchpad, 0x2E1) == 1) ? 0x28 : 0x39;
            B(Scratchpad, 0x2A2) = 0;
            func_8018E720(0, nextScreen);
            func_8018E4AC(3, 0x0F, 0);
            B(Scratchpad, 0x2DE) = D_801D8038;
        }
        return 1;

    default:
        /* 3..9, 11..19, 21..29, 33..39, 42..49, 54..59, 63..69 (.L801A2168) */
        return 0;
    }
}
