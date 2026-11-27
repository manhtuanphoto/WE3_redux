/*
 * func_80070348
 *
 * Purpose:
 *   State-machine handler for the in-match pause menu (initializes UI slot
 *   records, determines menu layout/item count, spawns menu text/icon widgets,
 *   processes D-pad/button navigation, camera zoom/speed and option cycling,
 *   and transitions to sub-menus or resumes play).
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   int: 0 while menu remains active / transitioning internally,
 *        2 when exiting/resuming match (item 7 selected, or Circle/Cancel/Start).
 *
 * Scratchpad (0x1F800000) reads/writes:
 *   - 0x29A (u8, R): match phase (used to map phase to overlay info index)
 *   - 0x2A1 (u8, R): mode/pause-type flag (1 = full pause menu, != 1 = replay/other)
 *   - 0x2CE (u8, R): flag affecting layout (0 vs 2 when 0x2E1 < 5)
 *   - 0x2E0 (u8, R): active side index (0 or 1; paired with ^1)
 *   - 0x2E1 (u8, R): game/match mode (5 = normal match / exhibition?)
 *   - 0x2E6 (u8, R/W): cycled option value (0..7)
 *   - 0x2F0 (u8, W): cleared to 0 when option 0x2E6 changes
 *   - 0x333 (u8, R): mode/flag selecting full 10-slot vs phase-specific widget init
 *   - 0x334 (u8[2], R): per-side values passed to func_8001CDAC
 *   - 0x336 (s16, R/W): camera distance/zoom setting, adjusted by +-4 and clamped to [0x1A0, 0x220] (416..544)
 *   - 0x1F0..0x20C (32 bytes, R) -> 0x210..0x22C (32 bytes, W): copied when 0x2E6 changes and 0x2A1 == 1
 *
 * Globals read/written:
 *   - D_8010A321 (u8, R/W): pause menu state (0..5), also accessed at D_800FF748 + 0xABD9
 *   - D_8010A320 (u8, W): selected action ID (accessed at D_800FF748 + 0xABD8)
 *   - D_8010A326 / D_8010A327 (u8, R): saved into D_800E6CC0[0..1] (at D_800FF748 + 0xABDE..0xABDF)
 *   - D_800E7DEA (u8, R/W): current menu cursor index
 *   - D_800EF839 (u8, R/W): menu layout index (0..3)
 *   - D_800FF728 (u8, R/W): number of menu items (3, 4, or 7)
 *   - D_800F0F72 (u8, R/W): flag indicating saved slot data is available
 *   - D_800E7608 (u16[8], R/W), D_800FF6F0 (u16[8+], R/W): widget parameter arrays
 *   - D_800FF748 + 0x5DC4 + idx*40 (R/W): 40-byte UI slot records
 *   - D_800FF748 + 0x6314 (u8, W): cleared if D_800E6CC0[0] == 1 and changed
 *   - D_8010A5A4 (u16, R/W): current controller button mask
 *   - D_8010A5DE (u8, W): dirty flag set on settings change
 *   - D_8010C1D0 (u8, R/W): confirmed menu action ID (2..7)
 *   - D_800F106E (u8, W): set to 1 in state 4
 *
 * Magic numbers / units:
 *   - Pad masks: 0x1000 = Up, 0x4000 = Down, 0x8000 = Left, 0x2000 = Right,
 *                0x20 = Circle (confirm), 0x40 = Cross (cancel), 0x800 = Start
 *   - 0x336 range [0x1A0, 0x220] (416..544), step +-4, mapped via 0x20 - (val - 0x1A0) / 4 -> 32..0
 *   - No physics position/speed units (UI/menu function; 30 Hz frame cadence).
 */

#define B(p, o)  (*(unsigned char *)((unsigned char *)(p) + (o)))
#define H(p, o)  (*(unsigned short *)((unsigned char *)(p) + (o)))
#define S(p, o)  (*(signed short *)((unsigned char *)(p) + (o)))
#define W(p, o)  (*(unsigned int *)((unsigned char *)(p) + (o)))
#define SW(p, o) (*(signed int *)((unsigned char *)(p) + (o)))
#define SCRATCH  ((unsigned char *)0x1F800000)

extern unsigned char D_800C7BB8[];
extern unsigned char D_800C7C14[];
extern unsigned char D_800CA3E8[];
extern unsigned char D_800CA3EA[];
extern unsigned char D_800CA3EC[];
extern unsigned char D_800CA3EE[];
extern unsigned char D_800CA3F0[];
extern unsigned char D_800CA3F2[];
extern unsigned char D_800CA3F4[];
extern unsigned char D_800CA420[];
extern unsigned char D_800E6CC0[];
extern unsigned char D_800E7608[];
extern unsigned char D_800E7DEA[];
extern unsigned char D_800EF839[];
extern unsigned char D_800F0F72[];
extern unsigned char D_800F106E[];
extern unsigned char D_800FF6F0[];
extern unsigned char D_800FF6F2[];
extern unsigned char D_800FF6F4[];
extern unsigned char D_800FF6F6[];
extern unsigned char D_800FF6F8[];
extern unsigned char D_800FF6FA[];
extern unsigned char D_800FF6FC[];
extern unsigned char D_800FF6FE[];
extern unsigned char D_800FF728[];
extern unsigned char D_800FF740[];
extern unsigned char D_800FF748[];
extern unsigned char D_8010A321[];
extern unsigned char D_8010A5A4[];
extern unsigned char D_8010A5DE[];
extern unsigned char D_8010C1D0[];

extern int func_800198E8(unsigned int);
extern void func_8001CDAC(int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int);
extern void func_8001CEE0(int, int, int, int, int, int, int, int);
extern unsigned char *func_8001D004(int);
extern void func_80020C14(void);
extern int func_8004FB68(int);
extern int func_8004FBC4(int);
extern unsigned short func_8006EC80(unsigned int);
extern void func_8006F4A8(int, int);
extern void func_8006FA60(int, int);
extern void func_8006FDD4(void);
extern unsigned int func_8006FFC4(void);
extern void func_80072904(void);
extern void func_800AB620(int);
extern void func_800B6D78(void);

int func_80070348(void)
{
    int result = 0;
    unsigned int state = B(D_8010A321, 0);
    unsigned char *base = D_800FF748;
    unsigned char *scratch = SCRATCH;

    if (state >= 6) {
        return result;
    }

    switch (state) {
    case 0: {
        unsigned char saved0 = B(base, 0xABDE); /* 0x8010A326 */
        unsigned char saved1 = B(base, 0xABDF); /* 0x8010A327 */
        unsigned char mode333 = B(scratch, 0x333);
        unsigned int i = 0;

        B(D_8010A5DE, 0) = 0;
        B(D_800F0F72, 0) = 0;
        B(D_800E7DEA, 0) = 0;
        B(D_800E6CC0, 0) = saved0;
        B(D_800E6CC0, 1) = saved1;

        if (mode333 != 0) {
            for (i = 0; (i & 0xFF) < 10; i++) {
                unsigned int idx = i & 0xFF;
                unsigned char *slot = base + (idx + 0x1A) * 40;
                if (B(slot, 0x5DC4) != 0) {
                    H(D_800E7608, idx * 2) = H(slot, 0x5DE0);
                    B(D_800F0F72, 0) = 1;
                    H(D_800FF6F0, idx * 2) = H(slot, 0x5DD0);
                }
            }
        } else if (B(scratch, 0x29A) == 0) {
            if (B(base, 0x61D4) != 0) {
                B(D_800F0F72, 0) = 1;
                H(D_800E7608, 0) = H(base, 0x61EE);
                H(D_800E7608, 2) = H(base, 0x61F0);
            }
        } else {
            for (i = 0; (i & 0xFF) < 10; i++) {
                unsigned int idx = i & 0xFF;
                unsigned char *slot = base + (idx + 0x1A) * 40;
                if (B(slot, 0x5DC4) != 0) {
                    B(D_800F0F72, 0) = 1;
                    H(D_800FF6F0, idx * 2) = H(slot, 0x5DD0);
                }
            }
        }

        B(base, 0xABD9) = B(base, 0xABD9) + 1; /* D_8010A321++ */
        return result;
    }

    case 1: {
        unsigned int itemCount;
        if (B(scratch, 0x2A1) == 1) {
            unsigned int mode = B(scratch, 0x2E1);
            if (mode == 4) {
                B(D_800EF839, 0) = 2;
                itemCount = 3;
            } else if ((int)mode < 5) {
                if (B(scratch, 0x2CE) != 0) {
                    B(D_800EF839, 0) = 2;
                    itemCount = 3;
                } else {
                    B(D_800EF839, 0) = 0;
                    itemCount = 7;
                }
            } else if (mode == 5) {
                B(D_800EF839, 0) = 1;
                itemCount = 7;
            } else {
                B(base, 0xABD9) = B(base, 0xABD9) + 1;
                return result;
            }
        } else {
            B(D_800EF839, 0) = 3;
            itemCount = 4;
        }
        B(D_800FF728, 0) = (unsigned char)itemCount;
        B(base, 0xABD9) = B(base, 0xABD9) + 1;
        return result;
    }

    case 2: {
        unsigned int layout = B(D_800EF839, 0);
        int arg4 = (layout == 2) ? 0x24 : 0;
        unsigned char *obj;
        unsigned int i;
        unsigned int activeIdx;

        func_8001CEE0(7, B(D_800FF740, 0) + 0x50, 0, 0, arg4, 8, 0, B(scratch, 0x2A1));

        layout = B(D_800EF839, 0);
        if (layout == 2) {
            func_8001CEE0(9, 0x5B, 0, 0, 0x1B, 8, 0x5C, 1);
            func_8001CEE0(10, 0x56, 0, 0, -0x2D, 8, 0x5C, 1);
            func_8001CEE0(11, 0x58, 0, 0, -0x2D, 8, 0x5C, 1);
            func_8001CEE0(18, 0x5E, 0, 0, 0x1B, 8, 0, 1);
            func_8001CEE0(19, 0x5E, 0, 0, 0x2D, 8, 0, 1);
            func_8001CEE0(20, 0x5E, 0, 0, 0x3F, 8, 0, 1);
        } else {
            unsigned int mode2E1 = B(scratch, 0x2E1);
            int strId = (mode2E1 == 5) ? 0x5D : 0x5B;
            int val;

            func_8001CEE0(8, strId, 0, 0, 0, 8, 0x5C, S(D_800CA3E8, layout * 14));

            if (B(scratch, 0x2A1) == 0) {
                strId = (B(scratch, 0x2E1) == 5) ? 0x5C : 0x5A;
            } else {
                strId = S(D_800CA3EA, B(D_800EF839, 0) * 14) + 0x51;
            }
            func_8001CEE0(9, strId, 0, 0, 0, 8, 0x5C, 1);

            layout = B(D_800EF839, 0);
            arg4 = (layout != 0) ? 0x12 : 0;
            func_8001CEE0(10, S(D_800CA3EC, layout * 14) + 0x51, 0, 0, arg4, 8, 0x5C, 1);

            layout = B(D_800EF839, 0);
            arg4 = (layout == 3) ? -0x24 : (int)(layout * 18);
            func_8001CEE0(11, S(D_800CA3EE, layout * 14) + 0x51, 0, 0, arg4, 8, 0x5C, 1);

            layout = B(D_800EF839, 0);
            func_8001CEE0(12, S(D_800CA3F0, layout * 14) + 0x51, 0, 0, 0, 8, 0x5C, S(D_800CA3F2, layout * 14));

            layout = B(D_800EF839, 0);
            if (layout != 3) {
                obj = func_8001D004(S(D_800CA3F0, layout * 14) + 0x51);
                val = S(scratch, 0x336);
                B(obj, 0x19) = (unsigned char)(0x20 - (val - 0x1A0) / 4);
            }

            if (B(scratch, 0x2A1) == 0) {
                strId = (B(scratch, 0x2E1) == 5) ? 0x59 : 0x58;
                func_8001CEE0(13, strId, 0, 0, -0x24, 8, 0x5C, 1);
            } else {
                layout = B(D_800EF839, 0);
                func_8001CEE0(13, S(D_800CA3F2, layout * 14) + 0x51, 0, 0, 0, 8, 0x5C, 1);
            }

            layout = B(D_800EF839, 0);
            func_8001CEE0(14, S(D_800CA3F4, layout * 14) + 0x51, 0, 0, 0, 8, 0x5C, S(D_800CA3F4, layout * 14));

            func_8001CEE0(16, 0x5E, 0, 0, 0, 8, 0, S(D_800CA3E8, B(D_800EF839, 0) * 14));
            func_8001CEE0(17, 0x5E, 0, 0, 0x12, 8, 0, 1);
            func_8001CEE0(18, 0x5E, 0, 0, 0x24, 8, 0, 1);
            func_8001CEE0(19, 0x5E, 0, 0, 0x36, 8, 0, 1);
            func_8001CEE0(20, 0x5E, 0, 0, 0x48, 8, 0, 1);
            func_8001CEE0(21, 0x5E, 0, 0, 0x5A, 8, 0, S(D_800CA3F2, B(D_800EF839, 0) * 14));
            func_8001CEE0(22, 0x5E, 0, 0, 0x6C, 8, 0, S(D_800CA3F4, B(D_800EF839, 0) * 14));

            obj = func_8001D004(0x54);
            if (B(scratch, 0x2E6) < 5) {
                B(obj, 0x24) = 0xFF;
                B(obj, 0x25) = 0x1C;
                B(obj, 0x28) = (unsigned char)((B(scratch, 0x2E6) << 4) - 0x80);
                if ((unsigned int)(B(scratch, 0x2E6) - 3) < 2) {
                    B(obj, 0x25) = 0x20;
                    H(obj, 0x2A) = 0x1A;
                } else {
                    H(obj, 0x2A) = 0x22;
                }
            } else {
                B(obj, 0x24) = 0;
                B(obj, 0x25) = 0x10;
                H(obj, 0x2A) = 0x16;
                B(obj, 0x28) = (unsigned char)((B(scratch, 0x2E6) << 4) - 0xD0);
            }
        }

        if (B(scratch, 0x2E1) == 5) {
            obj = func_8001D004(0x52);
            B(obj, 0x4) = (unsigned char)((B(base, 0xABDE) << 4) + 0x40);
        }

        func_8001CEE0(15, 0x5F, 0, 0, 0, 8, 0, 1);
        func_8001CEE0(25, 0x60, 0, 0, 0, 8, 0, 1);

        obj = func_8001D004(0x60);
        layout = B(D_800EF839, 0);
        {
            unsigned int isCompact = (layout < 2);
            unsigned int offset = (3 - layout) * 9;
            int leftX = (int)offset - 0x25;
            int rightX = 0x29 - (int)offset;

            for (i = 0; (i & 0xFF) < 4; i++) {
                unsigned int idx = i & 0xFF;
                int xPos;
                if (idx < 2) {
                    xPos = isCompact ? -0x40 : leftX;
                } else {
                    xPos = isCompact ? 0x44 : rightX;
                }
                H(obj, idx * 12 + 8) = (unsigned short)xPos;
            }
        }

        if (B(D_800EF839, 0) >= 2) {
            for (i = 0; (i & 0xFF) < 16; i++) {
                unsigned char *slot = base + ((i & 0xFF) + 8) * 40;
                if (B(slot, 0x5DC4) != 0) {
                    H(slot, 0x5DD2) += 9;
                }
            }
        }

        {
            int yBase = (int)B(D_800E7DEA, 0) * 18;
            int yPos;
            layout = B(D_800EF839, 0);
            if (layout < 2) {
                yPos = yBase + 0xFFC7; /* 0xFFC7 = (u16)(-57) */
            } else {
                yPos = (int)((3 - layout) * 9) + (yBase + 0xFFE2); /* 0xFFE2 = (u16)(-30) */
            }
            func_8006F4A8(15, yPos);
        }

        activeIdx = 0;
        for (i = 0; (i & 0xFF) < 8; i++) {
            unsigned int slotIdx = (i & 0xFF) + 8;
            if (B(base, 0x5DC4 + slotIdx * 40) != 0) {
                if ((activeIdx & 0xFF) == B(D_800E7DEA, 0)) {
                    func_8006FA60((int)slotIdx, 15);
                }
                activeIdx++;
            }
        }

        if (B(D_800F0F72, 0) == 0) {
            B(base, 0xABD9) = B(base, 0xABD9) + 1;
            return result;
        }

        if (B(scratch, 0x333) != 0) {
            unsigned int side = B(scratch, 0x2E0);
            func_8001CDAC(0x1A, 0x7F, 0, H(D_800FF6F0, 0), 0, 7, 0, 0x1000, H(D_800E7608, 0), 0, 0, 0x80, 0x80, 0x80, 0, B(scratch, 0x334 + side));
            func_8001CDAC(0x1B, 0x80, 0, H(D_800FF6F2, 0), 0, 7, 0, 0x1000, H(D_800E7608, 2), 0, 0, 0x80, 0x80, 0x80, 0, B(scratch, 0x334 + (side ^ 1)));
            func_8001CDAC(0x1C, 0x7D, 0, H(D_800FF6F4, 0), 0, 0xA, 0, 0x1000, H(D_800E7608, 4), 0, 0, 0x80, 0x80, 0x80, 0, B(scratch, 0x334 + B(scratch, 0x2E0)));
            func_8001CDAC(0x1D, 0x7E, 0, H(D_800FF6F6, 0), 0, 0xA, 0, 0x1000, H(D_800E7608, 6), 0, 0, 0x80, 0x80, 0x80, 0, B(scratch, 0x334 + (B(scratch, 0x2E0) ^ 1)));
            func_8001CDAC(0x1E, 0x7B, 0, H(D_800FF6F8, 0), 0, 6, 0, 0x1000, H(D_800E7608, 8), 0, 0, 0x80, 0x80, 0x80, 0, B(scratch, 0x334 + B(scratch, 0x2E0)));
            func_8001CDAC(0x1F, 0x7C, 0, H(D_800FF6FA, 0), 0, 6, 0, 0x1000, H(D_800E7608, 10), 0, 0, 0x80, 0x80, 0x80, 0, B(scratch, 0x334 + (B(scratch, 0x2E0) ^ 1)));
            func_8001CDAC(0x20, 0x81, 0, H(D_800FF6FC, 0), 0, 0xA, 0, 0x1000, H(D_800E7608, 12), 0, 0, 0x80, 0x80, 0x80, 0, B(scratch, 0x334 + B(scratch, 0x2E0)));
            func_8001CDAC(0x21, 0x82, 0, H(D_800FF6FE, 0), 0, 0xA, 0, 0x1000, H(D_800E7608, 14), 0, 0, 0x80, 0x80, 0x80, 0, B(scratch, 0x334 + (B(scratch, 0x2E0) ^ 1)));
        } else {
            unsigned int phase = B(scratch, 0x29A);
            unsigned int infoIdx;

            switch (phase) {
            case 0:
                infoIdx = 9;
                break;
            case 3:
                infoIdx = 0;
                break;
            case 4:
                infoIdx = 2;
                break;
            case 9:
                infoIdx = 1;
                break;
            case 12:
                infoIdx = 3;
                break;
            default:
                infoIdx = 0xFF;
                break;
            }

            if (infoIdx != 0xFF) {
                if (infoIdx == 9) {
                    func_8001CDAC(0x1A, 0x23, 0, 0, 0, 6, 0, H(D_800E7608, 0), H(D_800E7608, 2), 0, 0, 0x80, 0x80, 0x80, 0, 1);
                } else {
                    unsigned int count = B(D_800C7BB8, infoIdx);
                    if (count != 0) {
                        unsigned char *strTable = D_800C7C14 + infoIdx * 10;
                        for (i = 0; (i & 0xFF) < B(D_800C7BB8, infoIdx); i++) {
                            unsigned int idx = i & 0xFF;
                            func_8001CEE0((int)(idx + 0x1A), (int)B(strTable, idx) - 0x38, 0, H(D_800FF6F0, idx * 2) + 8, 0, 6, 0, 1);
                        }
                    }
                }
            }
        }

        B(base, 0xABD9) = B(base, 0xABD9) + 1;
        return result;
    }

    case 3: {
        unsigned short btn;
        unsigned int i;
        unsigned int activeIdx;

        if (func_8004FB68(0) == 0) {
            return result;
        }

        btn = func_8006EC80(func_8006FFC4() & 0xFF);
        H(D_8010A5A4, 0) = btn;

        if (btn == 0x1000 || btn == 0x4000) {
            activeIdx = 0;
            for (i = 0; (i & 0xFF) < 7; i++) {
                unsigned int slotIdx = (i & 0xFF) + 8;
                if (B(base, 0x5DC4 + slotIdx * 40) != 0) {
                    if ((activeIdx & 0xFF) == B(D_800E7DEA, 0)) {
                        func_8006FA60((int)slotIdx, 0x5C);
                    }
                    activeIdx++;
                }
            }
        }

        btn = H(D_8010A5A4, 0);
        if (btn == 0x1000) {
            /* D-pad Up: move cursor up with wrap-around */
            unsigned int cur = B(D_800E7DEA, 0);
            unsigned int next = (cur == 0) ? (unsigned int)(B(D_800FF728, 0) - 1) : (cur - 1);
            B(D_800E7DEA, 0) = (unsigned char)next;
            func_800AB620(0x50D);
        } else if (btn == 0x4000) {
            /* D-pad Down: move cursor down with wrap-around */
            int maxIdx = (int)B(D_800FF728, 0) - 1;
            int cur = (int)B(D_800E7DEA, 0);
            unsigned int next = (cur < maxIdx) ? (unsigned int)(cur + 1) : 0;
            B(D_800E7DEA, 0) = (unsigned char)next;
            func_800AB620(0x50D);
        } else if (btn == 0x2000 || btn == 0x8000) {
            /* D-pad Left (0x8000) / Right (0x2000): adjust option value */
            int item = S(D_800CA420, B(D_800EF839, 0) * 14 + B(D_800E7DEA, 0) * 2);
            if (item == 1) {
                unsigned char val = B(scratch, 0x2E6);
                unsigned int next;
                unsigned char *obj;

                if (H(D_8010A5A4, 0) == 0x8000) {
                    next = (val == 0) ? 7 : (unsigned int)(val - 1);
                } else {
                    next = (val < 7) ? (unsigned int)(val + 1) : 0;
                }
                B(scratch, 0x2E6) = (unsigned char)next;
                B(scratch, 0x2F0) = 0;

                if (B(scratch, 0x2A1) == 1) {
                    W(scratch, 0x210) = W(scratch, 0x1F0);
                    W(scratch, 0x214) = W(scratch, 0x1F4);
                    W(scratch, 0x218) = W(scratch, 0x1F8);
                    W(scratch, 0x21C) = W(scratch, 0x1FC);
                    W(scratch, 0x220) = W(scratch, 0x200);
                    W(scratch, 0x224) = W(scratch, 0x204);
                    W(scratch, 0x228) = W(scratch, 0x208);
                    W(scratch, 0x22C) = W(scratch, 0x20C);
                }

                obj = func_8001D004(0x54);
                if (B(scratch, 0x2E6) < 5) {
                    B(obj, 0x24) = 0xFF;
                    B(obj, 0x25) = 0x1C;
                    B(obj, 0x28) = (unsigned char)((B(scratch, 0x2E6) << 4) - 0x80);
                    if ((unsigned int)(B(scratch, 0x2E6) - 3) < 2) {
                        B(obj, 0x25) = 0x20;
                        H(obj, 0x2A) = 0x1A;
                    } else {
                        H(obj, 0x2A) = 0x22;
                    }
                } else {
                    B(obj, 0x24) = 0;
                    B(obj, 0x25) = 0x10;
                    H(obj, 0x2A) = 0x16;
                    B(obj, 0x28) = (unsigned char)((B(scratch, 0x2E6) << 4) - 0xD0);
                }

                func_80020C14();
                func_80072904();
            } else if (item == 8) {
                int zoom = S(scratch, 0x336);
                unsigned char *obj;

                if (H(D_8010A5A4, 0) == 0x8000) {
                    zoom += 4;
                } else {
                    zoom -= 4;
                }
                S(scratch, 0x336) = (signed short)zoom;
                zoom = S(scratch, 0x336);
                if (zoom < 0x1A0) {
                    zoom = 0x1A0;
                } else if (zoom >= 0x221) {
                    zoom = 0x220;
                }
                S(scratch, 0x336) = (signed short)zoom;

                obj = func_8001D004(S(D_800CA3F0, B(D_800EF839, 0) * 14) + 0x51);
                B(obj, 0x19) = (unsigned char)(0x20 - (S(scratch, 0x336) - 0x1A0) / 4);
            }
        } else if (btn == 0x20) {
            /* Circle: confirm selected menu item */
            unsigned int action = H(D_800CA420, B(D_800EF839, 0) * 14 + B(D_800E7DEA, 0) * 2);
            B(D_8010C1D0, 0) = (unsigned char)action;
            action &= 0xFF;

            switch (action) {
            case 2:
            case 5:
            case 6:
                func_800AB620(0x528);
                B(base, 0xABD9) = 0;                 /* D_8010A321 = 0 */
                B(base, 0xABD8) = B(D_8010C1D0, 0);  /* D_8010A320 */
                break;
            case 3:
            case 4:
                B(base, 0xABD9) = B(base, 0xABD9) + 1;
                func_800AB620(0x528);
                break;
            case 7:
                func_800AB620(0x528);
                result = 2;
                break;
            default:
                break;
            }

            if (B(scratch, 0x2E1) == 5) {
                unsigned char prev0 = B(D_800E6CC0, 0);
                if (prev0 != B(base, 0xABDE) || B(D_800E6CC0, 1) != B(base, 0xABDF)) {
                    if (prev0 == 1) {
                        B(base, 0x6314) = 0;
                    }
                    B(D_8010A5DE, 0) = 1;
                }
            }
        } else if (btn == 0x40 || btn == 0x800) {
            /* Cross (0x40, when 0x2A1 != 0) or Start (0x800): unpause / exit */
            if (btn != 0x40 || B(scratch, 0x2A1) != 0) {
                if (B(scratch, 0x2E1) == 5) {
                    unsigned char prev0 = B(D_800E6CC0, 0);
                    if (prev0 != B(base, 0xABDE) || B(D_800E6CC0, 1) != B(base, 0xABDF)) {
                        if (prev0 == 1) {
                            B(base, 0x6314) = 0;
                        }
                        B(D_8010A5DE, 0) = 1;
                    }
                }
                func_800AB620(0x529);
                result = 2;
            }
        }

        btn = H(D_8010A5A4, 0);
        if (btn == 0x1000 || btn == 0x4000) {
            int yBase = (int)B(D_800E7DEA, 0) * 18;
            unsigned int layout = B(D_800EF839, 0);
            int yPos;

            if (layout < 2) {
                yPos = yBase + 0xFFC7; /* 0xFFC7 = (u16)(-57) */
            } else {
                yPos = (int)((3 - layout) * 9) + (yBase + 0xFFE2); /* 0xFFE2 = (u16)(-30) */
            }
            func_8006F4A8(15, yPos);

            activeIdx = 0;
            for (i = 0; (i & 0xFF) < 7; i++) {
                unsigned int slotIdx = (i & 0xFF) + 8;
                if (B(base, 0x5DC4 + slotIdx * 40) != 0) {
                    if ((activeIdx & 0xFF) == B(D_800E7DEA, 0)) {
                        func_8006FA60((int)slotIdx, 15);
                    }
                    activeIdx++;
                }
            }
        }
        return result;
    }

    case 4:
        if (func_8004FBC4(0) == 0) {
            return result;
        }
        func_8006FDD4();
        B(D_800F106E, 0) = 1;
        func_800B6D78();
        B(base, 0xABD9) = B(base, 0xABD9) + 1;
        return result;

    case 5:
        if (func_800198E8(B(D_8010C1D0, 0)) != 0) {
            B(base, 0xABD9) = 0;
            B(base, 0xABD8) = B(D_8010C1D0, 0);
        }
        return result;
    }

    return result;
}
