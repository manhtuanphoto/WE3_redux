/*
 * func_800189F0
 *
 * Purpose:
 *   Multi-stage match/mode asset loading and setup state machine (dispatched on
 *   SCRATCH+0x29F: stages 0..10). Waits on CD/async load checks (func_80017684),
 *   rasterizes team strings (func_80198688), queues team/kit/stadium/ball/HUD
 *   resources (func_80019564), uploads VRAM textures (func_80050380,
 *   func_80050510), and returns non-zero once loading is complete.
 *
 * Arguments:
 *   None
 *
 * Return value:
 *   int - 0 while loading/waiting; non-zero when stage 9 (in mode 1) or
 *         stage 10 finishes.
 *
 * Reads / Writes:
 *   Scratchpad (0x1F800000):
 *     +0x29F (u8)  loading stage index 0..10 (R/W)
 *     +0x2DD (u8)  home team ID (R/W)
 *     +0x2DE (u8)  away team ID (R/W)
 *     +0x2E1 (u8)  game mode, 1 = simplified/practice mode (R)
 *     +0x2E3 (u8)  stadium / weather / time-of-day parameter (R/W)
 *     +0x2E4 (u8)  stadium sub-parameter (R/W)
 *     +0x2E5 (u8)  pitch / lighting parameter (R/W)
 *   Globals:
 *     D_800E6BF4 (u8[2])  kit/banner variant selectors (W/R)
 *     D_800E6BF6 (u16)    extra team resource offset (0 or 0xB5) (W/R)
 *     D_800E765C (u32)    VRAM / texture dest X (W)
 *     D_800E7668 (u32)    VRAM / texture dest Y (W)
 *     D_800F106E (u8)     stage-advance / busy flag, set to 1 (W)
 *     D_800F3564 (u8)     VRAM upload override flag, set to 1 (W)
 *     D_800FF748 (+0x8F15..+0x8F19, +0x8F1F, +0xABA0, +0xABA8) match config / kit flags (R/W)
 *     D_8010A3E8 (u32)    CLUT / upload param, set to 0xFF (W)
 *     D_8010C1D8 (+0x0C)  selected match/kit variant byte (R/W)
 *     D_8010CE20 (u8)     weather/stadium audio or asset bank selector (W/R)
 *     D_8014A000 (u8*)    common match asset pointer (R)
 *     D_801A6000..D_801A6118 (u8*[]) loaded file/resource pointer tables (R)
 *
 * Magic numbers:
 *   No spatial/physics units (asset/VRAM coordinates and resource IDs only).
 *   0x24 (36) written temporarily to SCRATCH+0x2DE around func_80198688.
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define SCRATCH ((unsigned char*)0x1F800000)

extern unsigned char D_800E6BF4[2];
extern unsigned short D_800E6BF6;
extern unsigned int D_800E765C;
extern unsigned int D_800E7668;
extern unsigned char D_800F106E;
extern unsigned char D_800F3564;
extern unsigned char D_800FF748[];
extern unsigned int D_8010A3E8;
extern unsigned char D_8010C1D8[];
extern unsigned char D_8010CE20;
extern unsigned char *D_8014A000;
extern unsigned char *D_801A6000[];
extern unsigned char *D_801A6004[];
extern unsigned char *D_801A6008[];
extern unsigned char *D_801A600C;
extern unsigned char *D_801A6010[];
extern unsigned char *D_801A6018[];
extern unsigned char *D_801A6020[];
extern unsigned char *D_801A6024;
extern unsigned char *D_801A6028[];
extern unsigned char *D_801A6038[];
extern unsigned char *D_801A603C;
extern unsigned char *D_801A6040;
extern unsigned char *D_801A6064[];
extern unsigned char *D_801A6114;
extern unsigned char *D_801A6118;

extern void func_800B6D78(void);
extern void func_80017564(unsigned int selector);
extern int func_80017684(int id);
extern void func_80019564(unsigned char *ptr, unsigned int arg1);
extern int func_800198E8(int arg0);
extern void func_80019B1C(void);
extern void func_80050380(unsigned int selector);
extern void func_80050510(unsigned int mode);
extern void func_80198688(void);

int func_800189F0(void)
{
    unsigned int stage = B(SCRATCH, 0x29F);

    if (stage >= 11U) {
        return 0;
    }

    switch (stage) {
    case 0: {
        unsigned int mode = B(SCRATCH, 0x2E1);
        B(D_800FF748, 0x8F15) = 0;
        if (mode != 1U && (B(D_800FF748, 0x8F1F) & 0x40) != 0) {
            unsigned char away = B(SCRATCH, 0x2DE);
            if (B(D_800FF748, 0xABA0) == away) {
                unsigned char t1 = B(D_800FF748, 0x8F19);
                unsigned char t2 = B(D_800FF748, 0x8F17);
                B(SCRATCH, 0x2DD) = away;
                B(D_800FF748, 0x8F18) = t1;
                B(D_800FF748, 0x8F16) = t2;
            } else {
                B(SCRATCH, 0x2DE) = B(SCRATCH, 0x2DD);
            }
        } else {
            unsigned char home = B(SCRATCH, 0x2DD);
            B(D_800FF748, 0x8F16) = 0;
            B(D_800FF748, 0x8F18) = 0;
            B(SCRATCH, 0x2DE) = home;
        }

        B(SCRATCH, 0x2E4) = 0;
        func_800B6D78();
        func_80017564(0xBE);
        func_80019564(D_8014A000, 0);
        B(SCRATCH, 0x2DE) = 0x24;
        func_80198688();
        B(SCRATCH, 0x2DE) = B(SCRATCH, 0x2DD);
        func_80019B1C();

        D_800F106E = 1;
        D_800E6BF6 = 0;
        B(SCRATCH, 0x29F) = B(SCRATCH, 0x29F) + 1;

        if (B(SCRATCH, 0x2E1) == 1) {
            D_800E6BF4[0] = 0;
            B(D_8010C1D8, 0xC) = 0;
            D_800E6BF4[1] = 0;
        } else {
            unsigned int variant = B(D_800FF748, 0x8F1F) & 0xF;
            switch (variant) {
            case 0:
                D_800E6BF4[0] = 3;
                B(D_8010C1D8, 0xC) = 3;
                D_800E6BF4[1] = 1;
                B(SCRATCH, 0x2E3) = 5;
                if (B(SCRATCH, 0x2DD) == 2) {
                    D_800E6BF6 = 0xB5;
                }
                B(SCRATCH, 0x2E5) = 1;
                break;
            case 1:
                D_800E6BF4[0] = 1;
                B(D_8010C1D8, 0xC) = 2;
                D_800E6BF4[1] = 2;
                break;
            case 2:
                D_800E6BF4[0] = 2;
                B(D_8010C1D8, 0xC) = 1;
                D_800E6BF4[1] = 3;
                break;
            case 3:
                D_800E6BF4[0] = 2;
                B(D_8010C1D8, 0xC) = 6;
                D_800E6BF4[1] = 6;
                break;
            case 4:
                D_800E6BF4[0] = 2;
                B(D_8010C1D8, 0xC) = 4;
                D_800E6BF4[1] = 5;
                break;
            case 5:
                D_800E6BF4[0] = 2;
                B(D_8010C1D8, 0xC) = 5;
                D_800E6BF4[1] = 4;
                break;
            default:
                break;
            }
        }

        D_8010CE20 = 0;
        if (B(SCRATCH, 0x2E5) != 0 && B(SCRATCH, 0x2E3) == 0) {
            D_8010CE20 = 7;
        }
        if (B(SCRATCH, 0x2E1) == 1 && B(D_800FF748, 0xABA8) == 0) {
            B(SCRATCH, 0x29F) = 2;
        }
        return 0;
    }

    case 1:
        if ((func_80017684(0xB) & 0xFF) == 0) {
            return 0;
        }
        func_80019564(D_801A6000[0], 0);
        func_80019564(D_801A6000[1 + D_800E6BF4[1]], 0);
        func_80019564(D_801A6000[8 + D_800E6BF4[1]], 0);
        switch (B(D_8010C1D8, 0xC)) {
        case 0:
        case 1:
        case 2:
        case 4:
            func_80019564(D_801A603C, 0);
            break;
        case 3:
        case 5:
        case 6:
        default:
            func_80019564(D_801A6040, 0);
            break;
        }
        func_80019564(D_801A6064[0], 0);
        func_80019564(D_801A6064[1 + B(SCRATCH, 0x2DD)], 0);
        func_80019564(D_801A6064[(int)D_800E6BF4[0] - 8], 0);
        func_80019564(D_801A6064[(int)D_800E6BF4[0] - 4], 0);
        func_80019564(D_801A6114, 0);
        func_80019564(D_801A6118, 0);
        D_800F106E = 1;
        B(SCRATCH, 0x29F) = B(SCRATCH, 0x29F) + 1;
        return 0;

    case 2: {
        int i;
        unsigned char slot, kit, flag;
        int home = B(SCRATCH, 0x2DD);

        if ((func_80017684(home * 3 + 0x13) & 0xFF) == 0) {
            return 0;
        }

        D_800F3564 = 1;
        D_800E765C = 0x200;
        D_800E7668 = 0x100;
        slot = B(D_800FF748, 0x8F15);
        kit = B(D_800FF748, 0x8F16 + slot);
        func_80019564(D_801A6000[kit], 0);

        D_800F3564 = 1;
        D_800E765C = 0x240;
        D_800E7668 = 0x180;
        slot = B(D_800FF748, 0x8F15);
        kit = B(D_800FF748, 0x8F16 + slot);
        func_80019564(D_801A6004[kit], 0);

        D_800E7668 = 0x180;
        D_800E765C = 0x340;
        D_800F3564 = 1;
        slot = B(D_800FF748, 0x8F15);
        flag = B(D_800FF748, 0x8F18 + slot);
        func_80019564((flag == 1) ? D_801A6010[0] : D_801A6000[1], 0);

        D_800E7668 = 0x180;
        D_800E765C = 0x380;
        D_800F3564 = 1;
        slot = B(D_800FF748, 0x8F15);
        flag = B(D_800FF748, 0x8F18 + slot);
        func_80019564((flag == 1) ? D_801A600C : D_801A6000[0], 0);

        func_80050380(0);

        D_800F3564 = 1;
        D_800E765C = 0;
        D_800E7668 = 0x1E0;
        D_8010A3E8 = 0xFF;
        slot = B(D_800FF748, 0x8F15);
        kit = B(D_800FF748, 0x8F16 + slot);
        func_80019564(D_801A6008[kit], 0);

        D_800F3564 = 1;
        D_800E765C = 0x180;
        D_800E7668 = 0x100;
        slot = B(D_800FF748, 0x8F15);
        flag = B(D_800FF748, 0x8F18 + slot);
        func_80019564(D_801A6018[flag], 0);

        for (i = 0; i < 4; i++) {
            D_800F3564 = 1;
            D_800E765C = 0;
            D_800E7668 = 0x1E0 + (unsigned int)i;
            D_8010A3E8 = 0xFF;
            slot = B(D_800FF748, 0x8F15);
            flag = B(D_800FF748, 0x8F18 + slot);
            func_80019564(D_801A6028[flag], 0);
        }

        D_800F3564 = 1;
        D_800E765C = 0x280;
        D_800E7668 = 0x100;
        func_80019564(D_801A6020[0], 0);

        D_800F3564 = 1;
        D_800E765C = 0x280;
        D_800E7668 = 0x140;
        func_80019564(D_801A6020[0], 0);

        D_800F3564 = 1;
        D_800E765C = 0;
        D_800E7668 = 0x1FA;
        D_8010A3E8 = 0xFF;
        func_80019564(D_801A6024, 0);

        D_800F3564 = 1;
        D_800E765C = 0;
        D_800E7668 = 0x1FB;
        D_8010A3E8 = 0xFF;
        func_80019564(D_801A6024, 0);

        D_800F106E = 1;
        B(SCRATCH, 0x29F) = B(SCRATCH, 0x29F) + 1;
        if (B(SCRATCH, 0x2E1) == 1 && B(D_800FF748, 0xABA8) == 0) {
            B(SCRATCH, 0x29F) = 7;
        }
        return 0;
    }

    case 3: {
        int id = B(SCRATCH, 0x2E3) * 4 + 0x9A + B(SCRATCH, 0x2E5) * 2 + B(SCRATCH, 0x2E4);
        if ((func_80017684(id) & 0xFF) == 0) {
            return 0;
        }
        func_80019564(D_801A6000[0], 0);
        func_80019564(D_801A6004[0], 0);
        func_80019564(D_801A6008[0], 0);
        func_80019564(D_801A600C, 0);
        D_800F106E = 1;
        B(SCRATCH, 0x29F) = B(SCRATCH, 0x29F) + 2;
        func_80050510(1);
        return 0;
    }

    case 4:
        /* UNCLEAR: jump table entry 4 branches directly to function epilogue without setting $v0 */
        return 0;

    case 5: {
        unsigned int e3;
        if ((func_80017684(0xC) & 0xFF) == 0) {
            return 0;
        }
        e3 = B(SCRATCH, 0x2E3);
        switch (e3) {
        case 2:
            func_80019564(D_801A6008[B(SCRATCH, 0x2E5)], 0);
            break;
        case 3:
            func_80019564(D_801A6000[B(SCRATCH, 0x2E5)], 0);
            break;
        default:
            break;
        }
        e3 = B(SCRATCH, 0x2E3);
        if (e3 != 5U) {
            func_80019564(D_801A6010[e3 * 2U + B(SCRATCH, 0x2E5)], 0);
        }
        func_80019564(D_801A6038[B(SCRATCH, 0x2E5)], 0);
        D_800F106E = 1;
        B(SCRATCH, 0x29F) = B(SCRATCH, 0x29F) + 1;
        return 0;
    }

    case 6:
        if ((func_80017684(D_8010CE20 + 0x92 + B(SCRATCH, 0x2E3)) & 0xFF) != 0) {
            D_800F106E = 1;
            B(SCRATCH, 0x29F) = B(SCRATCH, 0x29F) + 1;
        }
        return 0;

    case 7: {
        int id = (int)((D_800E6BF6 + B(SCRATCH, 0x2DD) * 3 + 0x11) & 0xFFFFU);
        if ((func_80017684(id) & 0xFF) == 0) {
            return 0;
        }
        D_800F106E = 1;
        B(SCRATCH, 0x29F) = B(SCRATCH, 0x29F) + 1;
        if (B(SCRATCH, 0x2E1) == 1 && B(D_800FF748, 0xABA8) == 0) {
            B(SCRATCH, 0x29F) = 9;
        }
        return 0;
    }

    case 8:
        if ((func_80017684(B(D_8010C1D8, 0xC) + 0xB3) & 0xFF) != 0) {
            D_800F106E = 1;
            B(SCRATCH, 0x29F) = B(SCRATCH, 0x29F) + 1;
        }
        return 0;

    case 9: {
        int ret = func_80017684(0xA);
        if ((ret & 0xFF) == 0) {
            return 0;
        }
        D_800F106E = 1;
        B(SCRATCH, 0x29F) = B(SCRATCH, 0x29F) + 1;
        if (B(SCRATCH, 0x2E1) == 1) {
            return (B(D_800FF748, 0xABA8) < 1);
        }
        /* UNCLEAR: original code does not clear $v0 on this path, returning func_80017684(0xA) result */
        return ret;
    }

    case 10:
        return func_800198E8(1) & 0xFF;
    }

    return 0;
}
