/*
 * func_800749A4
 *
 * Purpose:
 *   Formation / substitution roster menu input handler for side/controller `slot`.
 *   Reads pad input via func_8006EC80, updates the 18-byte cursor state at D_800EF840[slot]
 *   (D_800F1068) for Up/Down/Left/Right/Confirm/Cancel/Start, handles player selection and
 *   lineup swaps/substitutions (checking red-card/unavailable flags in D_800EF789), updates
 *   cursor sprite positions, blinks the marked slot every 5 frames, and cycles sub-panel UI.
 *
 * Arguments:
 *   slot (unsigned int) - controller / team index (low byte used)
 *
 * Return value:
 *   int - 2 when Start (0x0800) is pressed to exit/confirm menu, 0 otherwise
 *
 * Reads / Writes:
 *   Scratchpad (0x1F800000):
 *     +0x290 (s32) frame counter (R)
 *     +0x2E1 (u8)  menu/game mode (R: 4 = scrollable 22-player substitution list)
 *   Globals:
 *     D_800EF840 (u8[])  18-byte cursor struct array per slot (R/W via D_800F1068)
 *     D_800F1068 (u8*)   pointer to active cursor struct (W/R):
 *       +0x00 (u16) cursor row, +0x02 (u16) selected flag, +0x04 (u16) blink state,
 *       +0x06 (u16) sub-panel toggle, +0x0A (u16) selected row,
 *       +0x0C (u16) scroll top, +0x0E (u16) scroll bottom
 *     D_8010A5A4 (u16)   last button mask (W/R)
 *     D_800FF748+0xABAA (u8[]) 22-byte team lineup slot table (R/W)
 *     D_800FF748+0x8E9C (u8[]) 22-byte player formation/role table (R/W)
 *     D_800EF789 (u8[])  4-byte stride x 22-player availability/card flag table (R)
 *
 * Magic numbers (30 frames/s):
 *   Pad bits: 0x1000=Up, 0x4000=Down, 0x8000=Left, 0x2000=Right, 0x0020=Circle, 0x0040=Cross, 0x0800=Start
 *   SFX ids: 0x50D=move, 0x512=cancel/error, 0x528=confirm, 0x529=deselect
 *   5 frames (0.167 s) blink interval on selected slot; 0xFFB4 = -76 px Y base offset
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define SCRATCH ((unsigned char*)0x1F800000)

extern unsigned char D_800EF840[];
extern unsigned char *D_800F1068;
extern unsigned short D_8010A5A4;
extern unsigned char D_800FF748[];
extern unsigned char D_800EF789[];
extern unsigned char D_800CA574[];
extern unsigned char D_800CA2F8[];
extern unsigned char D_800CA2FC[];

extern unsigned int func_8006EC80(unsigned int);
extern void func_8007524C(unsigned int);
extern void func_8006FAF4(unsigned int, unsigned int, int);
extern void func_800AB620(int);
extern unsigned char *func_8001D004(unsigned int);
extern void func_80075A14(unsigned int);
extern void func_8006F4A8(unsigned int, unsigned int);
extern void func_8006F528(unsigned int, unsigned char *, int, int);
extern int func_8006F714(unsigned int, unsigned char *, int);
extern int func_8006F624(unsigned int, unsigned char *, int);

int func_800749A4(unsigned int slot)
{
    unsigned int idx;
    unsigned char *cur;
    unsigned int btn;
    unsigned char *spr;
    unsigned char *lineup;
    unsigned char *flags_base;
    unsigned int val_cur;
    unsigned int val_sel;
    unsigned char *roles;
    unsigned char tmp_role;

    idx = slot & 0xFFU;
    D_800F1068 = D_800EF840 + idx * 18U;

    btn = func_8006EC80(idx) & 0xFFFFU;
    D_8010A5A4 = (unsigned short)btn;

    switch (btn) {
    case 0x4000:
        cur = D_800F1068;
        if (B(SCRATCH, 0x2E1) != 4) {
            if (H(cur, 0) < 0xAU) {
                H(cur, 0) = (unsigned short)(H(cur, 0) + 1U);
            } else {
                H(cur, 0) = 0;
            }
            func_800AB620(0x50D);
        } else if (H(cur, 0) == H(cur, 0xE)) {
            if (H(cur, 0) < 0x15U) {
                H(cur, 0xC) = (unsigned short)(H(cur, 0xC) + 1U);
                H(cur, 0xE) = (unsigned short)(H(cur, 0xE) + 1U);
                H(cur, 0) = (unsigned short)(H(cur, 0) + 1U);
                func_8007524C(idx);
                if (H(D_800F1068, 0xE) == 0x15U) {
                    func_8006FAF4(idx + 1U, 1U, 0x5B);
                } else {
                    func_8006FAF4(idx + 1U, 0U, 0xA);
                }
                func_800AB620(0x50D);
            }
        } else {
            H(cur, 0) = (unsigned short)(H(cur, 0) + 1U);
            func_800AB620(0x50D);
        }
        break;

    case 0x1000:
        cur = D_800F1068;
        if (B(SCRATCH, 0x2E1) != 4) {
            if (H(cur, 0) != 0) {
                H(cur, 0) = (unsigned short)(H(cur, 0) - 1U);
            } else {
                H(cur, 0) = 0xA;
            }
            func_800AB620(0x50D);
        } else if (H(cur, 0) == H(cur, 0xC)) {
            if (H(cur, 0) != 0) {
                H(cur, 0xC) = (unsigned short)(H(cur, 0xC) - 1U);
                H(cur, 0xE) = (unsigned short)(H(cur, 0xE) - 1U);
                H(cur, 0) = (unsigned short)(H(cur, 0) - 1U);
                func_8007524C(idx);
                if (H(D_800F1068, 0xC) == 0) {
                    func_8006FAF4(idx + 1U, 0U, 0x5B);
                } else {
                    func_8006FAF4(idx + 1U, 1U, 0xA);
                }
                func_800AB620(0x50D);
            }
        } else {
            H(cur, 0) = (unsigned short)(H(cur, 0) - 1U);
            func_800AB620(0x50D);
        }
        break;

    case 0x8000:
        cur = D_800F1068;
        if (B(SCRATCH, 0x2E1) == 4 && H(cur, 0xC) != 0) {
            H(cur, 0xC) = 0;
            H(cur, 0) = 0;
            H(cur, 0xE) = 0xA;
            spr = func_8001D004(idx + 0x6AU);
            B(spr, 0xA) = 0x5B;
            B(spr, 0x16) = 0xA;
            func_800AB620(0x50D);
        }
        break;

    case 0x2000:
        cur = D_800F1068;
        if (B(SCRATCH, 0x2E1) == 4 && H(cur, 0xC) < 0xBU) {
            H(cur, 0xC) = 0xB;
            H(cur, 0xE) = 0x15;
            H(cur, 0) = 0x15;
            spr = func_8001D004(idx + 0x6AU);
            B(spr, 0xA) = 0xA;
            B(spr, 0x16) = 0x5B;
            func_800AB620(0x50D);
        }
        break;

    case 0x0020:
        cur = D_800F1068;
        if (H(cur, 2) == 0) {
            H(cur, 2) = 1;
            H(cur, 0xA) = H(cur, 0);
            func_800AB620(0x528);
            break;
        }
        if (H(cur, 0xA) == H(cur, 0)) {
            H(cur, 2) = 0;
            func_80075A14(idx);
            func_800AB620(0x512);
            break;
        }

        lineup = D_800FF748 + 0xABAA + idx * 22U;
        flags_base = D_800EF789 + idx * 88U;
        H(cur, 2) = 0;
        val_cur = B(lineup, H(cur, 0));

        if (B(flags_base, (val_cur - 1U) * 4U) == 0) {
            val_sel = B(lineup, H(cur, 0xA));
            if (B(flags_base, (val_sel - 1U) * 4U) != 0) {
                if (val_cur < 0xCU && val_sel < 0xCU) {
                    B(lineup, H(cur, 0)) = (unsigned char)val_sel;
                    B(lineup, H(D_800F1068, 0xA)) = 1;
                    func_80075A14(idx);
                    func_8007524C(idx);
                    func_800AB620(0x528);
                } else {
                    H(D_800F1068, 2) = 0;
                    func_80075A14(idx);
                    func_800AB620(0x512);
                }
                break;
            }
        } else {
            val_sel = B(lineup, H(cur, 0xA));
            if (B(flags_base, (val_sel - 1U) * 4U) != 0) {
                goto swap_roles;
            }
        }

        idx = slot & 0xFFU;
        lineup = D_800FF748 + 0xABAA + idx * 22U;
        flags_base = D_800EF789 + idx * 88U;
        cur = D_800F1068;
        val_cur = B(lineup, H(cur, 0));
        if (B(flags_base, (val_cur - 1U) * 4U) != 0) {
            if (val_cur < 0xCU && B(lineup, H(cur, 0xA)) < 0xCU) {
                B(lineup, H(cur, 0xA)) = (unsigned char)val_cur;
                B(lineup, H(D_800F1068, 0)) = 1;
                func_80075A14(idx);
                func_8007524C(idx);
                func_800AB620(0x528);
            } else {
                H(D_800F1068, 2) = 0;
                func_80075A14(idx);
                func_800AB620(0x512);
            }
            break;
        }

swap_roles:
        roles = D_800FF748 + 0x8E9C + idx * 22U;
        lineup = D_800FF748 + 0xABAA + idx * 22U;
        cur = D_800F1068;
        val_cur = B(lineup, H(cur, 0));
        val_sel = B(lineup, H(cur, 0xA));
        tmp_role = B(roles, val_cur - 1U);
        B(roles, val_cur - 1U) = B(roles, val_sel - 1U);
        B(roles, val_sel - 1U) = tmp_role;
        func_80075A14(idx);
        func_8007524C(idx);
        func_800AB620(0x528);
        break;

    case 0x0040:
        cur = D_800F1068;
        if (H(cur, 2) != 0) {
            H(cur, 2) = 0;
            func_80075A14(idx);
            func_800AB620(0x529);
        }
        break;

    case 0x0800:
        func_800AB620(0x528);
        return 2;

    default:
        break;
    }

    btn = D_8010A5A4;
    if (btn == 0x1000U || btn == 0x4000U) {
        cur = D_800F1068;
        func_8006F4A8((slot & 0xFFU) + 0x12U, ((H(cur, 0) - H(cur, 0xC)) << 4) + 0xFFB4U);
        btn = D_8010A5A4;
    }

    if (btn == 0x8000U || btn == 0x2000U) {
        cur = D_800F1068;
        idx = slot & 0xFFU;
        func_8006F4A8(idx + 0x12U, ((H(cur, 0) - H(cur, 0xC)) << 4) + 0xFFB4U);
        func_8007524C(idx);
    }

    cur = D_800F1068;
    if (H(cur, 2) != 0) {
        if (H(cur, 0xA) != H(cur, 0)
            && (SW(SCRATCH, 0x290) % 5) == 0
            && H(cur, 0xA) >= H(cur, 0xC)
            && H(cur, 0xA) <= H(cur, 0xE)) {
            H(cur, 4) ^= 1U;
            if (H(cur, 4) != 0) {
                func_8006FAF4((slot & 0xFFU) + 0x10U, H(cur, 0xA) - H(cur, 0xC), 0xB);
            } else {
                func_8006FAF4((slot & 0xFFU) + 0x10U, H(cur, 0xA) - H(cur, 0xC), 8);
            }
        }
        if (H(D_800F1068, 2) != 0) {
            func_8006F528((slot & 0xFFU) + 0x12U, D_800CA574, 3, 1);
        }
    }

    if (H(D_800F1068, 6) == 0) {
        if (func_8006F714((slot & 0xFFU) + 1U, D_800CA2F8, 8) != 0) {
            H(D_800F1068, 6) ^= 1U;
        }
    } else {
        if (func_8006F624((slot & 0xFFU) + 1U, D_800CA2FC, 8) != 0) {
            H(D_800F1068, 6) ^= 1U;
        }
    }

    return 0;
}
