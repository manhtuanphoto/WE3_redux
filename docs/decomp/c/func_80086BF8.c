/*
 * func_80086BF8
 * Purpose: Substitution / squad-select menu state machine (4 phases in D_8010A322:
 *          0 = count active bench/squad players and init scroll state,
 *          1 = spawn UI widgets and show/hide scroll arrows,
 *          2 = handle D-pad up/down list scrolling and arrow blink animations,
 *          3 = clean up menu widgets and mark confirmation).
 * Arguments: none
 * Return value: int (1 when phase 3 completes, 0 otherwise)
 * Scratchpad reads/writes:
 *   - 0x1F800294 (s16/u16): cleared to 0 when scrolling list up/down
 *   - 0x1F8002E2 (u8): game/controller mode selector (determines 1-player vs 2-player scroll input)
 * Globals read/written:
 *   - D_8010A322 (u8, also accessed via D_800FF748 + 0xABDA): menu phase state (0..3)
 *   - D_800FF748 + 0x88FF + team*96 + slot*3 (u8): squad entry id (0xFF = empty)
 *   - D_800F0FF0[2] (18-byte structs per team):
 *       +0x00 (u16): current scroll offset
 *       +0x06 (u16): up-arrow blink toggle
 *       +0x08 (u16): down-arrow blink toggle
 *       +0x0A (u16, D_800F0FFA / D_800F100C): count of non-empty entries (<= 32)
 *   - D_8010A5A4 (u16): polled menu input mask (0x1000 = up, 0x4000 = down)
 * Magic numbers:
 *   - 0x20 (32) slots per team, 6 visible rows (scroll enabled when count >= 6, max offset = count - 5)
 *   - 0x50D: cursor move sound effect ID
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define SCRATCHPAD ((unsigned char *)0x1F800000)

extern unsigned char D_8010A322[];
extern unsigned char D_800FF748[];
extern unsigned char D_800F0FF0[];
extern unsigned char D_8010A5A4[];
extern unsigned char D_800CA5F0[];
extern unsigned char D_800CA5F4[];

extern int func_8004FBC4(int);
extern void func_8001CEE0(int, int, int, int, int, int, int, int);
extern void func_8006FA84(int, int);
extern unsigned char *func_8001D004(int);
extern void func_800873DC(int, int);
extern int func_8004FB68(int);
extern unsigned short func_8006EC80(int);
extern void func_800AB620(int);
extern int func_800525A4(void);
extern int func_8006F714(int, unsigned char *, int);
extern int func_8006F624(int, unsigned char *, int);

int func_80086BF8(void)
{
    unsigned char *scratch = SCRATCHPAD;
    unsigned char phase = B(D_8010A322, 0);

    switch (phase) {
    case 0: {
        unsigned int team;
        unsigned int slot;

        if (func_8004FBC4(0) == 0) {
            return 0;
        }

        for (team = 0; (team & 0xFF) < 2; team++) {
            unsigned int t = team & 0xFF;
            unsigned int squadOff = t * 96;  /* 32 entries * 3 bytes */
            unsigned int uiOff = t * 18;
            unsigned char *teamUi = D_800F0FF0 + uiOff;

            H(D_800F0FF0, uiOff + 0x0A) = 0;
            H(D_800F0FF0, uiOff + 0x00) = 0;
            H(D_800F0FF0, uiOff + 0x06) = 0;
            H(D_800F0FF0, uiOff + 0x08) = 0;

            for (slot = 0; (slot & 0xFF) < 32; slot++) {
                unsigned int s = slot & 0xFF;
                if (B(D_800FF748, 0x88FF + squadOff + s * 3) != 0xFF) {
                    H(teamUi, 0x0A) = H(D_800F0FF0, uiOff + 0x0A) + 1;
                }
            }
        }

        B(D_800FF748, 0xABDA) = B(D_800FF748, 0xABDA) + 1; /* D_8010A322++ */
        return 0;
    }

    case 1: {
        unsigned char *uiObj;

        func_8001CEE0(1, 0x89, 0, 0, 0, 6, 0, 1);
        func_8001CEE0(9, 0x94, 0, -24, 2, 7, 0, 1);
        func_8001CEE0(10, 0x95, 0, -44, 2, 7, 0, 1);
        func_8001CEE0(11, 0x96, 0, 26, 2, 9, 0, 1);
        func_8001CEE0(12, 0x97, 0, 4, 2, 9, 0, 1);
        func_8001CEE0(14, 0x9C, 0, -34, 2, 10, 0, 1);
        func_8001CEE0(15, 0x9D, 0, -54, 2, 10, 0, 1);
        func_8001CEE0(0x20, 0x98, 0x50000000, 0, 0, 10, 0, 0);
        func_8001CEE0(0x21, 0x99, 0x50000000, 0, 0, 10, 0, (H(D_800F0FF0, 0x0A) >= 6) ? 1 : 0);
        func_8001CEE0(0x22, 0x9A, 0x50000000, 0, 0, 10, 0, 0);
        func_8001CEE0(0x23, 0x9B, 0x50000000, 0, 0, 10, 0, (H(D_800F0FF0, 0x1C) >= 6) ? 1 : 0);
        func_8006FA84(5, 0);

        uiObj = func_8001D004(0x91);
        B(uiObj, 0xB4) = 0xFF;
        uiObj = func_8001D004(0x92);
        B(uiObj, 0x24) = 0xFF;

        func_800873DC(0, 0);
        func_800873DC(1, 0);
        B(D_8010A322, 0) = B(D_8010A322, 0) + 1;
        return 0;
    }

    case 2: {
        unsigned char mode2E2;
        unsigned int maxPollSide;
        unsigned int team;

        if (func_8004FB68(0) == 0) {
            return 0;
        }

        mode2E2 = B(scratch, 0x2E2);
        if (mode2E2 == 1) {
            maxPollSide = 1;
        } else if (mode2E2 == 0 || (mode2E2 >= 2 && mode2E2 < 5)) {
            maxPollSide = 0;
        }

        for (team = 0; (team & 0xFF) < 2; team++) {
            unsigned int t = team & 0xFF;
            unsigned short btn;
            unsigned int uiOff;

            if (t <= (maxPollSide & 0xFF)) {
                H(D_8010A5A4, 0) = func_8006EC80(t);
            }

            btn = H(D_8010A5A4, 0);
            uiOff = t * 18;

            if (btn == 0x1000) {
                /* Scroll up */
                if (H(D_800F0FF0, uiOff + 0x0A) >= 6) {
                    unsigned short cur = H(D_800F0FF0, uiOff);
                    if (cur != 0) {
                        H(D_800F0FF0, uiOff) = cur - 1;
                        func_800873DC(t, B(D_800F0FF0, uiOff));
                        if (H(D_800F0FF0, uiOff) == 0) {
                            func_8006FA84(t * 2 + 0x20, 0);
                        }
                        func_8006FA84(t * 2 + 0x21, 1);
                        H(scratch, 0x294) = 0;
                        func_800AB620(0x50D);
                    }
                }
            } else if (btn == 0x4000) {
                /* Scroll down */
                unsigned short count = H(D_800F0FF0, uiOff + 0x0A);
                if (count >= 6) {
                    signed int maxTop = (signed int)count - 5;
                    unsigned short cur = H(D_800F0FF0, uiOff);
                    if ((signed int)cur < maxTop) {
                        H(D_800F0FF0, uiOff) = cur + 1;
                        func_800873DC(t, B(D_800F0FF0, uiOff));
                        if (H(D_800F0FF0, uiOff) == H(D_800F0FF0, uiOff + 0x0A) - 5) {
                            func_8006FA84(t * 2 + 0x21, 0);
                        }
                        func_8006FA84(t * 2 + 0x20, 1);
                        H(scratch, 0x294) = 0;
                        func_800AB620(0x50D);
                    }
                }
            }
        }

        if (func_800525A4() != 0) {
            B(D_800FF748, 0xABDA) = B(D_800FF748, 0xABDA) + 1;
        }

        for (team = 0; (team & 0xFF) < 2; team++) {
            unsigned int t = team & 0xFF;
            unsigned int uiOff = t * 18;
            int toggled;

            if (H(D_800F0FF0, uiOff + 0x06) == 0) {
                toggled = func_8006F714(t + 0x20, D_800CA5F0, 4);
            } else {
                toggled = func_8006F624(t + 0x20, D_800CA5F4, 4);
            }
            if (toggled != 0) {
                H(D_800F0FF0, uiOff + 0x06) ^= 1;
            }

            if (H(D_800F0FF0, uiOff + 0x08) == 0) {
                toggled = func_8006F714(t + 0x22, D_800CA5F0, 4);
            } else {
                toggled = func_8006F624(t + 0x22, D_800CA5F4, 4);
            }
            if (toggled != 0) {
                H(D_800F0FF0, uiOff + 0x08) ^= 1;
            }
        }

        return 0;
    }

    case 3: {
        unsigned char *uiObj;
        unsigned int team;

        uiObj = func_8001D004(0x91);
        B(uiObj, 0xB4) = 0;
        uiObj = func_8001D004(0x92);
        B(uiObj, 0x24) = 0;

        for (team = 0; (team & 0xFF) < 2; team++) {
            unsigned int t = team & 0xFF;
            unsigned int uiOff = t * 18;
            unsigned short count = H(D_800F0FF0, uiOff + 0x0A);

            if (count != 0 && count < 5) {
                unsigned short curCount;

                uiObj = func_8001D004(t + 0x94);
                curCount = H(D_800F0FF0, uiOff + 0x0A);
                B(uiObj + curCount * 24, 0x0C) = 0;

                uiObj = func_8001D004(t + 0x9C);
                curCount = H(D_800F0FF0, uiOff + 0x0A);
                B(uiObj + curCount * 48, -0x0C) = 0;

                uiObj = func_8001D004(t + 0x96);
                curCount = H(D_800F0FF0, uiOff + 0x0A);
                B(uiObj + curCount * 48, -0x0C) = 0;
            }
        }

        return 1;
    }

    default:
        return 0;
    }
}
