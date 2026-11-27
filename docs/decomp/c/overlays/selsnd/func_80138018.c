/*
 * func_80138018 (selsnd overlay)
 *
 * Purpose:
 *   Sound Settings overlay menu (`SELSND`) state machine. Dispatches on
 *   `D_8010A321` (phases 0..3) to initialize audio-menu sprites/widgets,
 *   process D-pad navigation, option toggles, and continuous Left/Right volume
 *   slider adjustments (BGM, SE, Commentary), and handle menu exit/transition.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Scratchpad (0x1F800000) fields read / written:
 *   +0x294 (u16, R/W) - sub-phase frame delay timer (cleared in phase 0; waits 10 frames in phase 1 when +0x298 == 4)
 *   +0x298 (u8, R)    - top-level game/menu state (4 = in-match pause, 5 = pre-match/main menu)
 *   +0x2A1 (u8, R)    - pause/menu mode flag (1 triggers func_8006FF48(0xC) on exit when +0x298 != 4)
 *   +0x330 (u8, W)    - cleared to 0 on exit when +0x298 == 4
 *
 * Global fields read / written:
 *   D_800AD638 (u16, R/W)     - active menu cursor row (0..4: 0..2 = volume sliders, 3..4 = option toggles)
 *   D_800FF748 (u8[], R/W)    - main state/actor base (+0xABD9 aliases D_8010A321)
 *   D_8010A320 (u8, W)        - menu sub-mode (cleared to 0 on non-in-match exit)
 *   D_8010A321 (u8, R/W)      - sound menu state machine phase (0..3)
 *   D_8010A322 (u8, W)        - cleared to 0 on in-match exit
 *   D_8010A368 (u8, W)        - cleared to 0 in phase 0 when Scratchpad+0x298 == 5
 *   D_8010A369 (u8, W)        - cleared to 0 in phase 0 when Scratchpad+0x298 == 5
 *   D_8010A5A4 (u16, R/W)     - current auto-repeated controller button mask
 *   D_8010A5A8 (u8[], R/W)    - game/sound settings struct:
 *     +0x02 / D_8010A5AA (u8) - volume 0 level (0..32, applied via func_800A97A0(val >> 1))
 *     +0x03 / D_8010A5AB (u8) - volume 1 level (0..32, applied via func_800AB1A8(val >> 1))
 *     +0x04 / D_8010A5AC (u8) - volume 2 level (0..32, applied via func_800A984C(val >> 1))
 *     +0x05 (u8)              - toggle option 0 (row 3, 0/1)
 *     +0x06 (u8)              - toggle option 1 (row 4, 0/1)
 *   jtbl_80138000 (void*[])   - phase 2 row un-highlight jump table for D_800AD638 < 6
 *
 * Pad masks:
 *   0x0020 (Circle / confirm), 0x0040 (Cross / cancel),
 *   0x1000 (Up), 0x2000 (Right), 0x4000 (Down), 0x8000 (Left).
 */

#define B(p, o)  (*(unsigned char *)((unsigned char *)(p) + (o)))
#define H(p, o)  (*(unsigned short *)((unsigned char *)(p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned short D_800AD638;
extern unsigned char D_800FF748[];
extern unsigned char D_8010A320;
extern unsigned char D_8010A321;
extern unsigned char D_8010A322;
extern unsigned char D_8010A368;
extern unsigned char D_8010A369;
extern unsigned short D_8010A5A4;
extern unsigned char D_8010A5A8[];

extern void func_800171C4(int a0);
extern void func_800171FC(int a0);
extern int func_800174E0(unsigned int port, unsigned int pad, unsigned short mask);
extern void func_8001CEE0(int slot, int obj_id, int flags, int x, int y, int a5, int a6, int vis);
extern unsigned char *func_8001D004(int obj_id);
extern int func_8004FB68(int a0);
extern int func_8004FBC4(int a0);
extern void func_8004FD14(int a0);
extern unsigned short func_8006EC80(unsigned int mode);
extern void func_8006FA60(int slot, int pal_or_state);
extern void func_8006FAA8(int slot, int obj_id);
extern void func_8006FAF4(int slot, int idx, int pal_or_state);
extern void func_8006FEA8(void);
extern void func_8006FF48(int a0);
extern unsigned int func_8006FFC4(void);
extern void func_800A97A0(signed short level);
extern void func_800A984C(signed short idx);
extern void func_800AB1A8(signed short idx);
extern void func_800AB620(signed int cmd);

void func_80138018(void)
{
    unsigned int state = D_8010A321;

    switch (state) {
    case 0:
        if (B(Scratchpad, 0x298) == 5) {
            D_8010A369 = 0;
            D_8010A368 = 0;
        }
        D_800AD638 = 0;
        H(Scratchpad, 0x294) = 0;
        func_800AB620(0x11);
        D_8010A321 = (unsigned char)(D_8010A321 + 1);
        break;

    case 1: {
        unsigned char *obj1;
        unsigned char *obj0;
        unsigned int i;

        if (B(Scratchpad, 0x298) == 4) {
            unsigned short timer = H(Scratchpad, 0x294);
            H(Scratchpad, 0x294) = (unsigned short)(timer + 1);
            if (timer < 0xA) {
                return;
            }
        }

        /* Initialize row label, slider bar, toggle, icon, and background UI widgets */
        func_8001CEE0(0x01, 0x2006, 0, -4, 0x00, 7, 0x00, 1);
        func_8001CEE0(0x02, 0x2007, 0, -4, 0x07, 7, 0x00, 1);
        func_8001CEE0(0x03, 0x2008, 0, -4, 0x0E, 7, 0x00, 1);
        func_8001CEE0(0x04, 0x2009, 0, -4, 0x15, 7, 0x00, 1);
        func_8001CEE0(0x05, 0x200A, 0, -4, 0x1C, 7, 0x00, 1);
        func_8001CEE0(0x06, 0x200B, 0, -4, 0x00, 7, 0x00, 0);

        func_8001CEE0(0x0A, 0x2001, 0,  0, 0x00, 8, 0x00, 1);
        func_8001CEE0(0x0B, 0x2002, 0,  0, 0x15, 8, 0x85, 1);
        func_8001CEE0(0x0C, 0x2004, 0,  0, 0x1C, 8, 0x85, 1);
        func_8001CEE0(0x0D, 0x2013, 0,  0, 0x00, 8, 0x08, 0);

        func_8001CEE0(0x10, 0x200D, 0,  0, 0x00, 8, 0x81, 1);
        func_8001CEE0(0x11, 0x200E, 0,  0, 0x07, 8, 0x81, 1);
        func_8001CEE0(0x12, 0x200F, 0,  0, 0x0E, 8, 0x81, 1);
        func_8001CEE0(0x13, 0x2010, 0,  0, 0x15, 8, 0x81, 1);
        func_8001CEE0(0x14, 0x2011, 0,  0, 0x1C, 8, 0x81, 1);
        func_8001CEE0(0x15, 0x2012, 0,  0, 0x00, 8, 0x81, 0);

        func_8001CEE0(0x20, 0x200C, 0,  0, 0x00, 8, 0x00, 1);
        func_8001CEE0(0x25, 0x2000, 0,  0, 0x00, 8, 0x00, 1);
        func_8001CEE0(0x26, 0x2019, 0,  0, 0x00, 9, 0x00, 1);

        /* Scale initial volume levels (0..32) to slider bar widths (0..88) at 12-byte stride */
        obj1 = func_8001D004(0x2001);
        B(obj1, 0x01) = (unsigned char)(((signed int)B(D_8010A5A8, 2) * 11) / 4);
        B(obj1, 0x0D) = (unsigned char)(((signed int)B(D_8010A5A8, 3) * 11) / 4);
        B(obj1, 0x19) = (unsigned char)(((signed int)B(D_8010A5A8, 4) * 11) / 4);

        if (D_800AD638 < 3) {
            for (i = 0; (i & 0xFF) < 3; i++) {
                unsigned int row = i & 0xFF;
                if (row == D_800AD638) {
                    obj1 = func_8001D004(0x2001);
                    B(obj1, row * 12 + 0x0A) = 0x70;
                    obj0 = func_8001D004(0x2000);
                    B(obj0, row * 24 + 0x0A) = 0x80;
                    B(obj0, row * 24 + 0x16) = 0x80;
                } else {
                    obj1 = func_8001D004(0x2001);
                    B(obj1, row * 12 + 0x0A) = 0x71;
                    obj0 = func_8001D004(0x2000);
                    B(obj0, row * 24 + 0x0A) = 0x81;
                    B(obj0, row * 24 + 0x16) = 0x81;
                }
            }
        } else {
            func_8006FA60((int)D_800AD638 + 8, 0x84);
        }

        func_8006FA60((int)D_800AD638 + 1, 0x82);
        func_8006FA60((int)D_800AD638 + 0x10, 0x80);

        for (i = 0; (i & 0xFF) < 6; i++) {
            unsigned int row = i & 0xFF;
            func_8006FAF4(0x20, (int)row, (row == D_800AD638) ? 0x86 : 0x87);
        }

        func_8006FAA8(0x0B, (int)(B(D_8010A5A8, 5) ^ 1) + 0x2002);
        func_8006FAA8(0x0C, (int)B(D_8010A5A8, 6) + 0x2004);

        /* Reset cursor and advance to active input phase (D_800FF748+0xABD9 == D_8010A321) */
        D_800AD638 = 0;
        B(D_800FF748, 0xABD9) = (unsigned char)(B(D_800FF748, 0xABD9) + 1);
        break;
    }

    case 2: {
        unsigned short btn;
        unsigned int pad_mode;
        /* UNCLEAR: start_pad and end_pad are left uninitialized in asm if pad_mode > 2 */
        unsigned int pad = 0;
        unsigned int end_pad = 0;
        unsigned char *obj1;
        unsigned char *obj0;

        if (func_8004FB68(0) == 0) {
            return;
        }

        btn = func_8006EC80(func_8006FFC4());
        D_8010A5A4 = btn;

        /* On Up/Down press, un-highlight the currently selected row before moving cursor */
        if (btn == 0x1000 || btn == 0x4000) {
            unsigned short cur = D_800AD638;
            if (cur < 6) {
                switch (cur) {
                case 0:
                case 1:
                case 2:
                    obj1 = func_8001D004(0x2001);
                    B(obj1, D_800AD638 * 12 + 0x0A) = 0x71;
                    obj0 = func_8001D004(0x2000);
                    B(obj0, D_800AD638 * 24 + 0x0A) = 0x81;
                    B(obj0, D_800AD638 * 24 + 0x16) = 0x81;
                    break;
                case 3:
                case 4:
                    func_8006FA60((int)cur + 8, 0x85);
                    break;
                case 5:
                    func_8006FA60(0x0D, 0x08);
                    break;
                }
            }
            func_8006FA60((int)D_800AD638 + 1, 0x83);
            func_8006FA60((int)D_800AD638 + 0x10, 0x81);
            func_8006FAF4(0x20, (int)D_800AD638, 0x87);
        }

        /* Handle discrete D-pad / Circle / Cross button actions */
        switch (D_8010A5A4) {
        case 0x1000: /* Up */
            if (D_800AD638 == 0) {
                D_800AD638 = 4;
            } else {
                D_800AD638 = (unsigned short)(D_800AD638 - 1);
            }
            func_800AB620(0x50D);
            break;

        case 0x4000: /* Down */
            if (D_800AD638 < 4) {
                D_800AD638 = (unsigned short)(D_800AD638 + 1);
            } else {
                D_800AD638 = 0;
            }
            func_800AB620(0x50D);
            break;

        case 0x2000: /* Right */
        case 0x8000: /* Left */
            if (D_800AD638 == 3) {
                B(D_8010A5A8, 5) ^= 1;
                func_800AB620((B(D_8010A5A8, 5) == 0) ? 0x0D : 0x0C);
                func_8006FAA8(0x0B, (int)(B(D_8010A5A8, 5) ^ 1) + 0x2002);
                func_800AB620(0x50D);
            } else if (D_800AD638 == 4) {
                B(D_8010A5A8, 6) ^= 1;
                func_800AB620((B(D_8010A5A8, 6) == 0) ? 5 : 4);
                func_8006FAA8(0x0C, (int)B(D_8010A5A8, 6) + 0x2004);
                func_800AB620(0x50D);
            }
            break;

        case 0x0020: /* Circle (confirm) */
            func_800AB620(0x528);
            B(D_800FF748, 0xABD9) = (unsigned char)(B(D_800FF748, 0xABD9) + 1);
            break;

        case 0x0040: /* Cross (cancel) */
            func_800AB620(0x529);
            B(D_800FF748, 0xABD9) = (unsigned char)(B(D_800FF748, 0xABD9) + 1);
            break;
        }

        /* Determine active controller port range for continuous slider adjustment */
        pad_mode = func_8006FFC4();
        if (pad_mode == 0) {
            pad = 0;
            end_pad = 1;
        } else if (pad_mode == 1) {
            pad = 1;
            end_pad = 2;
        } else if (pad_mode == 2) {
            pad = 0;
            end_pad = 2;
        }

        for (; (pad & 0xFF) < (end_pad & 0xFF); pad++) {
            unsigned int port = pad & 0xFF;

            /* Held Right (0x2000): increase selected volume slider (0..32) */
            if (func_800174E0(port, 1, 0x2000) != 0) {
                obj1 = func_8001D004(0x2001);
                switch (D_800AD638) {
                case 0:
                    if (B(D_8010A5A8, 2) < 0x20) {
                        B(D_8010A5A8, 2) = (unsigned char)(B(D_8010A5A8, 2) + 1);
                        if (B(D_8010A5A8, 2) < 0x20) {
                            func_800A97A0((signed short)(B(D_8010A5A8, 2) >> 1));
                            B(obj1, 0x01) = (unsigned char)(((signed int)B(D_8010A5A8, 2) * 11) / 4);
                        }
                    }
                    break;
                case 1:
                    if (B(D_8010A5A8, 3) < 0x20) {
                        B(D_8010A5A8, 3) = (unsigned char)(B(D_8010A5A8, 3) + 1);
                        if (B(D_8010A5A8, 3) < 0x20) {
                            func_800AB1A8((signed short)(B(D_8010A5A8, 3) >> 1));
                            B(obj1, 0x0D) = (unsigned char)(((signed int)B(D_8010A5A8, 3) * 11) / 4);
                        }
                    }
                    break;
                case 2:
                    if (B(D_8010A5A8, 4) < 0x20) {
                        B(D_8010A5A8, 4) = (unsigned char)(B(D_8010A5A8, 4) + 1);
                        if (B(D_8010A5A8, 4) < 0x20) {
                            func_800A984C((signed short)(B(D_8010A5A8, 4) >> 1));
                            B(obj1, 0x19) = (unsigned char)(((signed int)B(D_8010A5A8, 4) * 11) / 4);
                        }
                    }
                    break;
                }
            }

            /* Held Left (0x8000): decrease selected volume slider */
            if (func_800174E0(port, 1, 0x8000) != 0) {
                obj1 = func_8001D004(0x2001);
                switch (D_800AD638) {
                case 0: {
                    unsigned char val = B(D_8010A5A8, 2);
                    if (val != 0) {
                        val = (unsigned char)(val - 1);
                        B(D_8010A5A8, 2) = val;
                        func_800A97A0((signed short)(val >> 1));
                        if ((val & 7) == 0) {
                            continue;
                        }
                    }
                    B(obj1, 0x01) = (unsigned char)(((signed int)B(D_8010A5A8, 2) * 11) / 4);
                    break;
                }
                case 1: {
                    unsigned char val = B(D_8010A5A8, 3);
                    if (val != 0) {
                        val = (unsigned char)(val - 1);
                        B(D_8010A5A8, 3) = val;
                        func_800AB1A8((signed short)(val >> 1));
                        if ((val & 7) == 0) {
                            continue;
                        }
                    }
                    B(obj1, 0x0D) = (unsigned char)(((signed int)B(D_8010A5A8, 3) * 11) / 4);
                    break;
                }
                case 2: {
                    unsigned char val = B(D_8010A5A8, 4);
                    if (val != 0) {
                        val = (unsigned char)(val - 1);
                        B(D_8010A5A8, 4) = val;
                        func_800A984C((signed short)(val >> 1));
                        if ((val & 7) == 0) {
                            continue;
                        }
                    }
                    B(obj1, 0x19) = (unsigned char)(((signed int)B(D_8010A5A8, 4) * 11) / 4);
                    break;
                }
                }
            }
        }

        /* Re-highlight the newly selected row after Up/Down navigation */
        if (D_8010A5A4 == 0x1000 || D_8010A5A4 == 0x4000) {
            if (D_800AD638 < 3) {
                obj1 = func_8001D004(0x2001);
                B(obj1, D_800AD638 * 12 + 0x0A) = 0x70;
                obj0 = func_8001D004(0x2000);
                B(obj0, D_800AD638 * 24 + 0x0A) = 0x80;
                B(obj0, D_800AD638 * 24 + 0x16) = 0x80;
            } else {
                func_8006FA60((int)D_800AD638 + 8, 0x84);
            }
            func_8006FA60((int)D_800AD638 + 1, 0x82);
            func_8006FA60((int)D_800AD638 + 0x10, 0x80);
            func_8006FAF4(0x20, (int)D_800AD638, 0x86);
        }
        break;
    }

    case 3:
        if (func_8004FBC4(0) == 0) {
            return;
        }
        func_8004FD14(1);
        if (B(Scratchpad, 0x298) == 4) {
            B(Scratchpad, 0x330) = 0;
            func_800171C4(0x10);
            func_800171FC(0);
            D_8010A322 = 0;
        } else {
            func_8006FEA8();
            if (B(Scratchpad, 0x2A1) == 1) {
                func_8006FF48(0x0C);
            }
            D_8010A320 = 0;
            D_8010A321 = 1;
        }
        break;
    }
}
