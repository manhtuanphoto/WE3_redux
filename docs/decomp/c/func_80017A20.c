/*
 * func_80017A20
 *
 * Purpose:
 *   Commentary, scoreboard banner, and match-end / halftime / goal-celebration
 *   presentation state machine (driven by Scratchpad+0x29F, states 0..16).
 *   Sequences commentary audio clips (func_80017684 / func_800198E8), sets up
 *   team crests/names/scores/minute sprites (func_80019564 / func_80050140),
 *   and signals completion to the caller when the presentation sequence finishes.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   0 while sequence steps are in progress (or state >= 17);
 *   1 (in case 8 when Scratchpad+0x298 == 3 and (D_800E7DE8 & 1) == 0);
 *   1 if D_800E6B84 == 0, else 0 (in case 11).
 *   (Cases 4 and 12..15 jump directly to epilogue without setting $v0.)
 *
 * Actor / global table fields read:
 *   D_800FF748 + 0x8F15 (u8) active/scoring side (0 or 1),
 *   D_800FF748 + 0x8F16[2] (u8) per-side banner/team index,
 *   D_800FF748 + 0x8F18[2] (u8) per-side win/result variant,
 *   D_800FF748 + 0x8F1F (u8) match flags (bit 0x40, low nibble 0x0F),
 *   D_800FF748 + 0x8F21 (u8) goal/result flags (shifted >> 1),
 *   D_8010C1D8 + 0x10 (u8) match minute (split into tens and ones digits),
 *   D_800E6B84 (s16) countdown/timer checked in case 11
 *
 * Global fields written:
 *   D_800FF748 + 0x8F15 (u8), D_800FF748 + 0x8F16 (u8),
 *   D_800E6BF0 / D_800E6BF2 (u16) per-side commentary/score offsets,
 *   D_800F106E (u8), D_8010CE20 (u8),
 *   D_800F3564 (u8), D_800E765C (u32), D_800E7668 (u32), D_8010A3E8 (u32)
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x298 (u8) game state (3 checked in case 8),
 *   +0x29F (u8) presentation step state (0..16),
 *   +0x2DD / +0x2DE (u8) team 0 / team 1 scores,
 *   +0x2E1 (u8) mode (0, 2, 5 checked),
 *   +0x2E3 (u8), +0x2E4 (u8), +0x2E5 (u8) match/half/extra-time sub-indices
 *
 * Scratchpad fields written:
 *   +0x29F (u8) presentation step state,
 *   +0x2E3 (u8) set to 6 in case 0 when +0x2E1 == 5,
 *   +0x2E4 (u8) cleared in case 0 when +0x2E1 == 5,
 *   +0x2E5 (u8) cleared in case 0 when +0x2E1 == 5
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   No spatial/frame physics constants; UI/sprite coordinates (0x100..0x380,
 *   0x1E0..0x1FB) and commentary clip IDs (0x11..0x13, 0x92, 0x9A, 0xB2, 0xB5,
 *   0xBF, 0xC0, 0xC3, 0xC5).
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern short D_800E6B84;
extern unsigned short D_800E6BF0[];
extern unsigned int D_800E765C;
extern unsigned int D_800E7668;
extern unsigned char D_800E7DE8;
extern unsigned char D_800F106E;
extern unsigned char D_800F3564;
extern unsigned char D_800FF748[];
extern unsigned int D_8010A3E8;
extern unsigned char D_8010C1D8[];
extern unsigned char D_8010CE20;
extern unsigned int D_801A6000[];
extern unsigned int D_801A6004[];
extern unsigned int D_801A6008[];
extern unsigned int D_801A600C;
extern unsigned int D_801A6010;
extern unsigned int D_801A6018[];
extern unsigned int D_801A6020;
extern unsigned int D_801A6024;
extern unsigned int D_801A6028[];
extern unsigned int D_801A6030[];

extern unsigned int func_80017684(unsigned int clipId);
extern void func_80019564(unsigned int spritePtr, int arg1);
extern unsigned int func_800198E8(int arg0);
extern int func_8001E6B8(int n);
extern void func_80050140(int arg0, int arg1, int arg2, int arg3);
extern void func_80050380(int side);
extern void func_80050510(int arg0);
extern void func_800AACB4(int score0, int score1);
extern void func_80189720(void);

int func_80017A20(void)
{
    unsigned char state = B(Scratchpad, 0x29F);
    int i;

    if (state >= 0x11) {
        return 0;
    }

    switch (state) {
    case 0:
        if (B(Scratchpad, 0x2E1) != 5) {
            func_800198E8(6);
            D_800F106E = 1;
            func_80189720();
        }
        D_800F106E = 1;
        B(Scratchpad, 0x29F) = B(Scratchpad, 0x29F) + 1;
        if (B(Scratchpad, 0x2E1) == 5) {
            B(Scratchpad, 0x2E3) = 6;
            B(Scratchpad, 0x2E5) = 0;
            B(Scratchpad, 0x2E4) = 0;
            B(D_800FF748, 0x8F15) = 0;
            if (func_8001E6B8(4) == 0) {
                B(D_800FF748, 0x8F16) = 3;
            } else {
                B(D_800FF748, 0x8F16) = 0;
            }
        }
        D_8010CE20 = 0;
        if (B(Scratchpad, 0x2E5) != 0 && B(Scratchpad, 0x2E3) == 0) {
            D_8010CE20 = 7;
        }
        return 0;

    case 1: {
        unsigned char side = B(D_800FF748, 0x8F15);
        unsigned char score = B(Scratchpad, 0x2DD + side);
        unsigned char minute;
        unsigned int yCoord;

        if ((func_80017684(score * 3 + 0x13) & 0xFF) == 0) {
            return 0;
        }

        D_800E6BF0[0] = 0;
        D_800E6BF0[1] = 0;

        if (B(Scratchpad, 0x2E1) == 2) {
            unsigned char flags = B(D_800FF748, 0x8F1F);
            if ((flags & 0x40) != 0 && (flags & 0x0F) == 0) {
                for (i = 0; i < 2; i++) {
                    if (B(Scratchpad, 0x2DD + i) == 2) {
                        D_800E6BF0[i] = 0xB5;
                    }
                }
            }
        }

        if (B(Scratchpad, 0x2E1) == 0) {
            switch (B(D_800FF748, 0x8F21) >> 1) {
            case 1:
                if (B(Scratchpad, 0x2DD) == 2) {
                    D_800E6BF0[0] = 0xB5;
                }
                break;
            case 2:
                if (B(Scratchpad, 0x2DE) == 2) {
                    D_800E6BF0[1] = 0xB5;
                }
                break;
            case 3:
                for (i = 0; i < 2; i++) {
                    if (B(Scratchpad, 0x2DD + i) == 2) {
                        D_800E6BF0[i] = 0xB5;
                    }
                }
                break;
            }
        }

        /* Display team 1 banner/score sprites */
        D_800F3564 = 1;
        D_800E765C = 0x200;
        D_800E7668 = 0x100;
        side = B(D_800FF748, 0x8F15);
        func_80019564(D_801A6000[B(D_800FF748, 0x8F16 + side)], 0);

        D_800F3564 = 1;
        D_800E765C = 0x240;
        D_800E7668 = 0x180;
        side = B(D_800FF748, 0x8F15);
        func_80019564(D_801A6004[B(D_800FF748, 0x8F16 + side)], 0);

        D_800E7668 = 0x180;
        D_800E765C = 0x340;
        D_800F3564 = 1;
        side = B(D_800FF748, 0x8F15);
        if (B(D_800FF748, 0x8F18 + side) == 1) {
            func_80019564(D_801A6010, 0);
        } else {
            func_80019564(D_801A6000[1], 0);
        }

        D_800E7668 = 0x180;
        D_800E765C = 0x380;
        D_800F3564 = 1;
        side = B(D_800FF748, 0x8F15);
        if (B(D_800FF748, 0x8F18 + side) == 1) {
            func_80019564(D_801A600C, 0);
        } else {
            func_80019564(D_801A6000[0], 0);
        }

        func_80050380(0);

        D_800F3564 = 1;
        D_800E765C = 0;
        D_800E7668 = 0x1E0;
        D_8010A3E8 = 0xFF;
        side = B(D_800FF748, 0x8F15);
        func_80019564(D_801A6008[B(D_800FF748, 0x8F16 + side)], 0);

        D_800F3564 = 1;
        D_800E765C = 0x180;
        D_800E7668 = 0x100;
        side = B(D_800FF748, 0x8F15);
        func_80019564(D_801A6018[B(D_800FF748, 0x8F18 + side)], 0);

        for (i = 0; i < 4; i++) {
            D_800F3564 = 1;
            D_800E765C = 0;
            D_800E7668 = 0x1E0 + i;
            D_8010A3E8 = 0xFF;
            side = B(D_800FF748, 0x8F15);
            func_80019564(D_801A6028[B(D_800FF748, 0x8F18 + side)], 0);
        }

        /* Draw tens and ones digits of match minute */
        minute = B(D_8010C1D8, 0x10);
        D_8010A3E8 = 0xFF;
        func_80019564(D_801A6030[minute / 10], 0);

        minute = B(D_8010C1D8, 0x10);
        func_80050140(((minute % 10) & 0xFF) * 16 + 0x10, 0x1E8, 0, 0x1EA);

        if (B(Scratchpad, 0x2E1) != 5) {
            side = B(D_800FF748, 0x8F15);
            D_800F3564 = 1;
            D_800E765C = 0x280;
            D_800E7668 = (side != 0) ? 0x140 : 0x100;
            func_80019564(D_801A6020, 0);

            side = B(D_800FF748, 0x8F15);
            D_800F3564 = 1;
            yCoord = (side != 0) ? 0x1FB : 0x1FA;
            D_800E765C = 0;
            D_800E7668 = yCoord;
            D_8010A3E8 = 0xFF;
            func_80019564(D_801A6024, 0);
        }

        D_800F106E = 1;
        B(Scratchpad, 0x29F) = B(Scratchpad, 0x29F) + 1;
        return 0;
    }

    case 2: {
        unsigned char otherSide = B(D_800FF748, 0x8F15) ^ 1;
        unsigned char score = B(Scratchpad, 0x2DD + otherSide);
        unsigned int yCoord;

        if ((func_80017684(score * 3 + 0x13) & 0xFF) == 0) {
            return 0;
        }

        /* Display team 2 banner/score sprites */
        D_800F3564 = 1;
        D_800E765C = 0x1C0;
        D_800E7668 = 0x180;
        otherSide = B(D_800FF748, 0x8F15) ^ 1;
        func_80019564(D_801A6000[B(D_800FF748, 0x8F16 + otherSide)], 0);

        D_800F3564 = 1;
        D_800E765C = 0x280;
        D_800E7668 = 0x180;
        otherSide = B(D_800FF748, 0x8F15) ^ 1;
        func_80019564(D_801A6004[B(D_800FF748, 0x8F16 + otherSide)], 0);

        D_800E7668 = 0x180;
        D_800E765C = 0x340;
        D_800F3564 = 1;
        otherSide = B(D_800FF748, 0x8F15) ^ 1;
        if (B(D_800FF748, 0x8F18 + otherSide) != 0) {
            func_80019564(D_801A6010, 0);
        } else {
            func_80019564(D_801A6000[1], 0);
        }

        D_800E7668 = 0x180;
        D_800E765C = 0x380;
        D_800F3564 = 1;
        otherSide = B(D_800FF748, 0x8F15) ^ 1;
        if (B(D_800FF748, 0x8F18 + otherSide) == 1) {
            func_80019564(D_801A600C, 0);
        } else {
            func_80019564(D_801A6000[0], 0);
        }

        func_80050380(1);

        D_800F3564 = 1;
        D_800E765C = 0;
        D_800E7668 = 0x1E4;
        D_8010A3E8 = 0xFF;
        otherSide = B(D_800FF748, 0x8F15) ^ 1;
        func_80019564(D_801A6008[B(D_800FF748, 0x8F16 + otherSide)], 0);

        D_800F3564 = 1;
        D_800E765C = 0x1C0;
        D_800E7668 = 0x100;
        otherSide = B(D_800FF748, 0x8F15) ^ 1;
        func_80019564(D_801A6018[B(D_800FF748, 0x8F18 + otherSide)], 0);

        for (i = 0; i < 4; i++) {
            D_800F3564 = 1;
            D_800E765C = 0;
            D_800E7668 = 0x1E4 + i;
            D_8010A3E8 = 0xFF;
            otherSide = B(D_800FF748, 0x8F15) ^ 1;
            func_80019564(D_801A6028[B(D_800FF748, 0x8F18 + otherSide)], 0);
        }

        if (B(Scratchpad, 0x2E1) != 5) {
            otherSide = B(D_800FF748, 0x8F15);
            D_800F3564 = 1;
            D_800E765C = 0x280;
            D_800E7668 = (otherSide != 0) ? 0x100 : 0x140;
            func_80019564(D_801A6020, 0);

            otherSide = B(D_800FF748, 0x8F15);
            D_800F3564 = 1;
            yCoord = (otherSide != 0) ? 0x1FA : 0x1FB;
            D_800E765C = 0;
            D_800E7668 = yCoord;
            D_8010A3E8 = 0xFF;
            func_80019564(D_801A6024, 0);
        }

        D_800F106E = 1;
        if (B(Scratchpad, 0x2E1) != 5) {
            B(Scratchpad, 0x29F) = B(Scratchpad, 0x29F) + 1;
        } else {
            B(Scratchpad, 0x29F) = 0x10;
        }
        return 0;
    }

    case 3: {
        unsigned int clipId = B(Scratchpad, 0x2E3) * 4 + 0x9A
                            + B(Scratchpad, 0x2E5) * 2
                            + B(Scratchpad, 0x2E4);
        if ((func_80017684(clipId) & 0xFF) == 0) {
            return 0;
        }
        func_80019564(D_801A6000[0], 0);
        func_80019564(D_801A6004[0], 0);
        func_80019564(D_801A6008[0], 0);
        func_80019564(D_801A600C, 0);
        func_80050140(0x60, 0x1FE, 0xF0, 0x1FC);
        if (B(Scratchpad, 0x2E1) == 2 && (B(D_800FF748, 0x8F1F) & 0x0F) == 0) {
            func_80050140(0x70, 0x1FE, 0xF0, 0x1FC);
        }
        D_800F106E = 1;
        B(Scratchpad, 0x29F) = B(Scratchpad, 0x29F) + 2;
        func_80050510(1);
        return 0;
    }

    case 4:
        /* UNCLEAR: jtbl_800100D8[4] branches directly to epilogue without setting $v0 */
        return 0;

    case 5:
        if ((func_80017684(D_8010CE20 + 0x92 + B(Scratchpad, 0x2E3)) & 0xFF) == 0) {
            return 0;
        }
        D_800F106E = 1;
        B(Scratchpad, 0x29F) = B(Scratchpad, 0x29F) + 1;
        return 0;

    case 6: {
        unsigned char side = B(D_800FF748, 0x8F15);
        unsigned char score = B(Scratchpad, 0x2DD + side);
        unsigned int clipId = (D_800E6BF0[side] + score * 3 + 0x11) & 0xFFFF;
        if ((func_80017684(clipId) & 0xFF) == 0) {
            return 0;
        }
        D_800F106E = 1;
        B(Scratchpad, 0x29F) = B(Scratchpad, 0x29F) + 1;
        return 0;
    }

    case 7: {
        unsigned char otherSide = B(D_800FF748, 0x8F15) ^ 1;
        unsigned char score = B(Scratchpad, 0x2DD + otherSide);
        unsigned int clipId = (D_800E6BF0[otherSide] + score * 3 + 0x12) & 0xFFFF;
        if ((func_80017684(clipId) & 0xFF) == 0) {
            return 0;
        }
        D_800F106E = 1;
        B(Scratchpad, 0x29F) = B(Scratchpad, 0x29F) + 1;
        return 0;
    }

    case 8:
        if (B(Scratchpad, 0x2E1) == 5) {
            if ((func_800198E8(9) & 0xFF) == 0) {
                return 0;
            }
            B(Scratchpad, 0x29F) = 0xA;
            return 0;
        }
        if (B(Scratchpad, 0x298) == 3) {
            unsigned char flag = D_800E7DE8;
            if ((flag & 1) == 0) {
                unsigned int clipId = ((flag & 3) == 2) ? 0xC5 : 0xC0;
                if ((func_80017684(clipId) & 0xFF) == 0) {
                    return 0;
                }
                return 1;
            }
            if (flag != 3) {
                if ((func_80017684(0xBF) & 0xFF) == 0) {
                    return 0;
                }
                B(Scratchpad, 0x29F) = 0xA;
                return 0;
            }
        }
        if ((func_800198E8(8) & 0xFF) == 0) {
            return 0;
        }
        D_800F106E = 1;
        B(Scratchpad, 0x29F) = B(Scratchpad, 0x29F) + 1;
        return 0;

    case 9:
        if ((func_80017684(0xC3) & 0xFF) == 0) {
            return 0;
        }
        B(Scratchpad, 0x29F) = B(Scratchpad, 0x29F) + 1;
        return 0;

    case 10:
        func_800AACB4(B(Scratchpad, 0x2DD) + 1, B(Scratchpad, 0x2DE) + 1);
        B(Scratchpad, 0x29F) = B(Scratchpad, 0x29F) + 1;
        return 0;

    case 11:
        return ((int)D_800E6B84 == 0);

    case 12:
    case 13:
    case 14:
    case 15:
        /* UNCLEAR: jtbl_800100D8[12..15] branch directly to epilogue without setting $v0 */
        return 0;

    case 16:
        if ((func_80017684(0xB2) & 0xFF) == 0) {
            return 0;
        }
        func_80019564(D_801A6000[0], 0);
        func_80019564(D_801A6004[0], 0);
        func_80019564(D_801A6008[0], 0);
        func_80019564(D_801A600C, 0);
        func_80050140(0x60, 0x1FE, 0xF0, 0x1FC);
        D_800F106E = 1;
        func_80050510(0);
        B(Scratchpad, 0x29F) = 5;
        return 0;
    }

    return 0;
}
