/*
 * func_8018F324 (overlay: select)
 *
 * Purpose:
 *   Main state machine for the Memory Card load/save/format/options menu in the
 *   `select` overlay, dispatched on `D_8010A322` (u8, states 0x00..0xF0 via
 *   `jtbl_80188FE0`). Manages UI sprite/panel setup (`D_80105508`, `D_801D8D90`,
 *   `D_801D8DA8`), memory card directory scanning and block counting, slot
 *   header reading (`func_80192A30`), data saving and read-back verification
 *   (`func_801930F4` / `func_80192A30`), card formatting (`func_800C5E4C`),
 *   error/confirmation dialogs (`func_801942E0`), and optional rule/mode
 *   editing (`D_8010A5A8[0..1]`, `Scratchpad+0x2E2`) when `Scratchpad+0x2E1 == 6`.
 *
 * Key Scratchpad (0x1F800000) offsets:
 *   +0x234 (u16, R)  : newly pressed pad 0 buttons
 *                      (0x0010=Triangle, 0x0020=Cross, 0x0800=Start,
 *                       0x1000=Up, 0x2000=Right, 0x4000=Down, 0x8000=Left)
 *   +0x294 (u16, R/W): frame delay / cooldown counter
 *   +0x29A (u8,  R)  : match phase / mode (0x22 triggers func_801B8708(1) after load)
 *   +0x2A2 (u8,  W)  : menu/overlay sub-flag (initialized to 0x82)
 *   +0x2E1 (u8,  R)  : memory card menu mode (6 = editable settings/save slot mode)
 *   +0x2E2 (u8,  R/W): option/setting value (0, 2, or 3 when edited on line 2)
 *   +0x330 (u8,  W)  : screen/fade active flag
 *
 * Key Globals:
 *   D_8010A322 (u8)  : primary state variable (0x00..0xF0; >= 0xF1 is no-op)
 *   D_8010A31E (s16) : screen fade level (0..0xFF)
 *   D_80108666 (u8)  : last selected memory card slot index (0..4)
 *   D_8010A5A8 (u8[2]): editable setting bytes [0] (0..5) and [1] (0..2)
 *   D_801D7D40 (s32) : next state after window animation (`0xD5`)
 *   D_801D7D44 (u8[2]): [0] = dialog window open state, [1] = error/confirm sub-mode
 *   D_801D7D56 (s16) : currently selected memory card slot (0..4)
 *   D_801D7DA0 (u8[5]): per-slot status (0=empty, 1=compatible, 0xFF=incompatible)
 *   D_801D8DFC (u8)  : 0 = load mode, nonzero = save mode
 *   D_801D8C68 / D_801D8A74 (s32): memory card used block count (15 = card full)
 */

#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;

typedef struct {
    u32 word0;
    u8  byte4;
    u8  padding5;
    u16 half6;
    u32 word8;
    u8 *pointer12;
    u16 half16, half18;
    u8  byte20, byte21, byte22, byte23, byte24, padding25;
    u16 half26, half28, half30, half32, padding34;
    u32 word36;
} Descriptor40; /* sizeof == 0x28 (40) */

typedef struct {
    u16 halves[4];
    u8  bytes[12];
} Fields20; /* sizeof == 0x14 (20) */

#define SCRATCHPAD_U8(off)  (*(volatile u8  *)(0x1F800000U + (off)))
#define SCRATCHPAD_U16(off) (*(volatile u16 *)(0x1F800000U + (off)))

/* Resident / shared globals (many accessed relative to D_800FF748 in asm) */
extern u8  D_800FF748[];
extern Descriptor40 D_80105508[]; /* Base at D_800FF748 + 0x5DC0 */
extern u8  D_80105CB4;            /* D_800FF748 + 0x656C */
extern u8  D_80108666;            /* D_800FF748 + 0x8F1E */
extern s16 D_8010A31E;            /* D_800FF748 + 0xABD6 */
extern u8  D_8010A322;            /* D_800FF748 + 0xABDA */
extern u8  D_8010A368;            /* D_800FF748 + 0xAC20 */
extern u8  D_8010A369;            /* D_800FF748 + 0xAC21 */
extern s16 D_8010A5A4;
extern u8  D_8010A5A8[];          /* 2 editable option bytes */
extern s16 D_800AD638;            /* Menu cursor row (u8/u16) */

/* Overlay text/dialog pointers */
extern u8 *D_8011D614;
extern u8 *D_8011D618;
extern u8 *D_8011D634;
extern u8 *D_8011D638;
extern u8 *D_8011D63C;
extern u8 *D_8011D640;
extern u8 *D_8011D644;
extern u8 *D_8011D648;
extern u8 *D_8011D64C;
extern u8 *D_8011D650;
extern u8 *D_8011D720;
extern u8 *D_8011D724;
extern u8 *D_801D58C4[];
extern u8 *D_801D590C;
extern u8 *D_801D5948;
extern u8 *D_801D595C;

/* Memory card buffers and slot tables */
extern void *D_801D1AFC;
extern u8  D_801D1B00[];
extern u8  D_801D1B11; /* D_801D1B00 + 0x11: slot index ASCII char ('0'..'4') */
extern u8  D_801D1B19;
extern u8  D_801D1B1A;
extern u8  D_801D1B1B;
extern u8  D_801D1B1C;
extern u8  D_801D1B1D; /* D_801D1B1C + 1 */
extern u32 D_801D1B20;
extern u8  D_801D1B24[];
extern u32 D_801D5A48;

extern s32 D_801D7D40;
extern u8  D_801D7D44[];
extern s32 D_801D7D4C;
extern s32 D_801D7D50;
extern u8  D_801D7D54;
extern s16 D_801D7D56;
extern u8  D_801D7D58;
extern u8  D_801D7D60[];
extern u8  D_801D7D68[];
extern u8  D_801D7D70[]; /* D_801D7D40 + 0x30 */
extern u8  D_801D7D78[];
extern u8  D_801D7D80[];
extern u8  D_801D7D88[]; /* D_801D7D58 + 0x30 */
extern u8  D_801D7DA0[];

extern s32 D_801D8A74;
extern s32 D_801D8C68;
extern Fields20 D_801D8D90;
extern Fields20 D_801D8DA8;
extern u8  D_801D8DBD;
extern u8  D_801D8DFC;
extern u8  D_801D8FB8;
extern u8  D_801D93BB;
extern u8  D_801DA070[];
extern u8  D_801DA2C8[];

/* Resident & overlay helper prototypes */
extern void func_8001CEE0(u8 first, u16 second, u32 third, s16 fourth,
                          s16 fifth, u8 sixth, u8 seventh, s16 eighth);
extern u8  *func_8001D004(u32 selector);
extern u32  func_800219F4(u8 *record, u32 code, u32 flag);
extern void func_8004FD14(u32 notify);
extern u16  func_8006EC80(u32 mode);
extern s32  func_8006F07C(u32 amount);
extern s32  func_8006F0DC(u32 step, u32 mode);
extern void func_8006F1EC(void);
extern void func_800AB620(s32 cmd);
extern s32  func_800ACEC8(void *name, void *dir_entry);
extern s32  func_800ACEE8(void *name);
extern u32  func_800BA518(s32 mode);
extern s32  func_800C5E4C(u32 channel);

extern void func_8018E2C0(void);
extern void func_8018E3B4(void);
extern void func_8018E4F0(void);
extern s32  func_80191E44(void);
extern void func_80191E80(void);
extern void func_80192174(void);
extern void func_80192334(u32 offset);
extern void func_80192428(void);
extern void func_80192594(u32 channel);
extern s32  func_80192720(u32 channel);
extern s32  func_80192848(s32 port);
extern void func_801929B4(u32 channel);
extern s32  func_80192A30(s32 mode, u32 channel, void *name, void *data, u32 size, u8 verify);
extern s32  func_801930F4(s32 mode, u32 channel, s32 arg3, void *name, void *data, u32 size);
extern s32  func_80193884(void);
extern void func_80193990(void);
extern void func_80193EA0(void);
extern void func_80193FB4(u8 index, u16 key, u32 word8, u16 half16, u16 half18,
                          u8 byte20, u8 byte21, u16 half30, u16 half32,
                          u16 half26, u16 half28, u8 byte22, u8 byte23, u8 byte24,
                          u32 word36, u8 word0, u8 byte4);
extern void func_801940EC(u8 index, u32 word, Fields20 *source, u8 byte2, u16 half0, u8 byte3);
extern void func_801941E0(u8 index, u32 word, Fields20 *source, u8 byte2, u16 half0, u8 byte3);
extern void func_801942E0(u8 index, u8 *pointer, u16 half2, u16 half4, u16 half6,
                          u8 byte8, u8 byte9, u8 byte10, u8 byte12, u8 byte11,
                          u16 half0, u8 byte16);
extern void func_801B8708(u32 arg0);

void func_8018F324(void)
{
    u8 state = D_8010A322;
    s32 i;
    s32 res;
    u16 pad_trig;

    if (state >= 0xF1U) {
        return;
    }

    switch (state) {
    case 0x00: {
        u8 mode;
        u8 prev_slot;

        D_801D8DBD = 0;
        func_8004FD14(0);
        func_8006F1EC();

        SCRATCHPAD_U8(0x330) = 1;
        D_8010A31E = 0xFF;
        D_801D5A48 = 0;

        mode = SCRATCHPAD_U8(0x2E1);
        SCRATCHPAD_U8(0x2A2) = 0x82;
        D_801D7D44[0] = 0;
        D_801D7D44[1] = 0;

        if (mode == 6) {
            func_8001CEE0(4, 0x5005, 0, 0, 0, 0x18, 0, 1);
            D_80105508[4].word0 = 3;
            D_801D8DFC = 0;
        } else {
            func_8001CEE0(4, 0x5006, 0, 0, 0, 0x18, 0, 1);
            D_80105508[4].word0 = 3;
        }

        func_8001CEE0(5, 0x300C, 0, 0, 0, 0x18, 0, 1);
        D_80105508[5].word0 = 3;
        func_8001CEE0(1, 0x5001, 0, 0, 0, 0x1C, 0, 1);
        D_80105508[1].word0 = 3;
        func_8001CEE0(3, 0x5004, 0, 0, 0, 0x17, 0, 1);
        D_80105508[3].word0 = 3;
        func_8001CEE0(6, 0x5002, 0, 0, 1, 6, 0, 1);
        D_80105508[6].word0 = 3;
        func_8001CEE0(15, 0x5003, 0, 0, 0, 0x1B, 0, 1);
        D_80105508[15].word0 = 3;
        func_8001CEE0(16, 0x5000, 0, 0, 0, 0x1C, 0, 1);
        D_80105508[16].word0 = 3;

        func_80193FB4(0x35, 0x5009, 0, 0, 0, 9, 0xBA, 0x1000, 0x1000,
                      0, 0, 0x80, 0x80, 0x80, 0, 6, 1);

        D_801D8DA8.halves[0] = (u16)(s16)-0xA9;
        D_801D8DA8.halves[1] = 0x33;
        D_801D8DA8.halves[2] = 0x154;
        D_801D8DA8.halves[3] = 0x2C;
        D_801D8DA8.bytes[0] = 0;
        D_801D8DA8.bytes[1] = 0;
        D_801D8DA8.bytes[2] = 0x80;
        func_801940EC(0, 0, &D_801D8DA8, 0, 4, 1);

        D_801D8DA8.halves[0] = 0x15;
        D_801D8DA8.halves[1] = (u16)(s16)-0x45;
        D_801D8DA8.halves[2] = 0x97;
        D_801D8DA8.halves[3] = 0x5D;
        func_801940EC(1, 0, &D_801D8DA8, 0, 4, 1);

        prev_slot = D_80108666;
        D_8010A368 = 0;
        D_8010A369 = 0;
        SCRATCHPAD_U16(0x294) = 0x30;
        D_801D7D56 = (s16)prev_slot;

        func_80192594(0);
        func_801929B4(0);
        D_801D7D40 = 2;
        D_8010A322 = 0xD0;
        break;
    }

    case 0x01: {
        u16 delay = SCRATCHPAD_U16(0x294);
        s32 next_state = 2;
        s32 stride_off;
        s32 slot_char;

        if (delay != 0) {
            SCRATCHPAD_U16(0x294) = delay - 1;
            break;
        }

        D_801D7D54 = 0;
        D_801D7D44[1] = 0;
        for (i = 0x230; i >= 0; i -= 0x28) {
            D_801DA070[i] = 0;
        }

        D_80105508[1].byte4 = 1;
        D_80105CB4 = 0;
        func_80193EA0();

        D_801D8C68 = func_80192848(0);
        if (D_801D8C68 == 0) {
            func_80192594(0);
            do {
                res = func_80192720(0);
                D_801D7D4C = res;
            } while (res == 0);

            if (res == -3) {
                D_801D7D54 = 1;
                if (D_801D8DFC == 0) {
                    func_80192594(0);
                    D_801D7D40 = 0x50;
                    D_8010A322 = 0xD4;
                    break;
                }
            } else if (res == -2) {
                D_801D7D40 = 0x60;
                D_8010A322 = 0xD4;
                break;
            } else if (res == -1) {
                D_801D7D40 = 0x90;
                D_8010A322 = 0xD4;
                break;
            } else if (res != 1) {
                break;
            }
        }

        stride_off = 0;
        slot_char = '0';
        D_801D7D50 = 0;
        D_801D7D58 = 0;

        for (i = 0; i < 5; i++, slot_char++) {
            D_801D1B11 = (u8)slot_char;
            if (func_800ACEC8(D_801D1B00, D_801DA2C8) != 0) {
                u8 *rec_5003;
                u8 *rec_5002;
                u8  code_byte;

                func_801929B4(0);
                do {
                    res = func_80192A30(0, 0, D_801D1B00, D_801D1AFC, 0x100, 0);
                    D_801D7D4C = res;
                } while (res == 0);

                if (res == -4 || res == -1) {
                    next_state = 0x90;
                    goto set_d4_transition;
                }
                if (res == -2) {
                    next_state = 0x60;
                    goto set_d4_transition;
                }
                if (res == 1) {
                    func_80192334((u32)i);
                }

                rec_5003 = func_8001D004(0x5003);
                if (D_801D7D60[i] == 2) {
                    u8 val = D_801D7D88[i] & 7;
                    rec_5003[stride_off + 4] = (u8)(val * 11);
                } else {
                    rec_5003[stride_off + 4] = 0x42;
                }

                code_byte = D_801D7D70[i];
                D_801D7D50 = 1;

                if (SCRATCHPAD_U8(0x2E1) == 6 || SCRATCHPAD_U8(0x2E1) == D_801D7D60[i]) {
                    rec_5003[stride_off + 10] = 0xC6;
                    rec_5002 = func_8001D004(0x5002);
                    func_800219F4(rec_5002 + stride_off, code_byte, 0);
                    D_801D7DA0[i] = 1;
                    D_801D7D58++;
                } else {
                    rec_5003[stride_off + 10] = 0xA8;
                    rec_5002 = func_8001D004(0x5002);
                    func_800219F4(rec_5002 + stride_off, code_byte, 0);
                    D_801D7DA0[i] = 0xFF;
                }
            }
            stride_off += 0xC;
        }

        if (D_801D8DFC == 0) {
            if (D_801D7D58 == 0) {
                next_state = 0x50;
                goto set_d4_transition;
            }
            for (i = 0; i < 5; i++) {
                s16 slot = (s16)(D_801D7D56 % 5);
                D_801D7D56 = slot;
                if (D_801D7DA0[slot] == 1) {
                    break;
                }
                D_801D7D56 = slot + 1;
            }
        } else {
            if (D_801D7D50 == 0 && D_801D8C68 == 15) {
                next_state = 0x80;
                goto set_d4_transition;
            }
        }

        D_80105508[1].half18 = (u16)(D_801D7D56 * 20);
        func_80193990();
        func_80192594(0);
        next_state = 2;

set_d4_transition:
        D_801D7D40 = next_state;
        D_8010A322 = 0xD4;
        break;
    }

    case 0x02: {
        u16 prev_slot;
        s32 step = 0; /* UNCLEAR: uninitialized in asm if neither Up nor Down is pressed, unused unless slot changes */
        s16 cur_slot;

        if (func_80193884() != 0) {
            break;
        }
        if (func_8006F07C(0x10) == 0) {
            break;
        }

        prev_slot = (u16)D_801D7D56;
        if (D_801D7D58 >= 2 || D_801D8DFC != 0) {
            if (SCRATCHPAD_U16(0x234) & 0x1000) {
                D_801D7D56 = (s16)(prev_slot + 4);
                step = 4;
                func_800AB620(0x50D);
            }
            if (SCRATCHPAD_U16(0x234) & 0x4000) {
                step = 1;
                D_801D7D56 = D_801D7D56 + 1;
                func_800AB620(0x50D);
            }

            if (D_801D8DFC != 0) {
                D_801D7D56 = (s16)(D_801D7D56 % 5);
            } else if ((s16)D_801D7D56 != (s32)(prev_slot & 0xFF)) {
                s32 prev_byte = prev_slot & 0xFF;
                for (i = 0; i < 5; i++) {
                    s16 mod_slot = (s16)(D_801D7D56 % 5);
                    D_801D7D56 = mod_slot;
                    if (D_801D7DA0[mod_slot] == 1 && mod_slot != prev_byte) {
                        break;
                    }
                    D_801D7D56 = (s16)(mod_slot + step);
                }
            }
        }

        cur_slot = D_801D7D56;
        D_80105508[1].half18 = (u16)(cur_slot * 20);
        if (cur_slot != (s32)(prev_slot & 0xFF)) {
            func_80193990();
        }

        if (D_801D7DA0[D_801D7D56] != 0) {
            if (SCRATCHPAD_U16(0x234) & 0x10) {
                func_801929B4(0);
                D_8010A322 = 0x40;
                func_800AB620(0x528);
                break;
            }
        } else {
            D_80105CB4 = 0;
        }

        if (func_80191E44() != 0) {
            D_8010A322 = 0xE0;
            break;
        }

        if (SCRATCHPAD_U16(0x234) & 0x20) {
            if (D_801D8DFC == 0) {
                D_8010A322 = 0x10;
            } else if (D_801D7DA0[D_801D7D56] != 0) {
                D_8010A322 = 0x30;
            } else {
                D_8010A322 = 0x20;
            }
            func_800AB620(0x528);
        }
        break;
    }

    case 0x03: {
        s16 slot;

        if (func_8006F0DC(0x10, 0) == 0) {
            break;
        }

        func_80193990();
        func_8001CEE0(2, 0x5008, 0x50000000U, 0, 0, 0x17, 0, 1);

        slot = D_801D7D56;
        D_80105508[2].word0 = 3;
        D_801D1B1B = D_801D7D80[slot];
        D_801D1B1C = D_801D7D78[slot];
        D_800AD638 = 0;
        D_8010A322++;
        D_801D1B1D = D_801D7D68[slot];
        break;
    }

    case 0x04: {
        u8 slot_type;
        u8 row;
        u8 opt_val;

        if (func_8006F07C(0x10) == 0) {
            break;
        }
        if (func_80193884() != 0) {
            D_80105508[2].byte4 = 0;
            break;
        }
        if (func_80191E44() != 0) {
            D_80105508[2].byte4 = 0;
            D_8010A322 = 0xD9;
            break;
        }

        pad_trig = SCRATCHPAD_U16(0x234);
        if (pad_trig & 0x800) {
            D_80105508[2].byte4 = 0;
            D_8010A322 = 0x30;
            func_800AB620(0x528);
            break;
        }

        slot_type = D_801D7D60[D_801D7D56];
        if (pad_trig & 0x1000) {
            if (slot_type == 1) {
                *(u8 *)&D_800AD638 += 2;
            } else {
                *(u8 *)&D_800AD638 += 1;
            }
            func_800AB620(0x50D);
        } else if (pad_trig & 0x4000) {
            func_800AB620(0x50D);
            *(u8 *)&D_800AD638 += 1;
        }

        if (D_801D7D60[D_801D7D56] == 1) {
            *(u8 *)&D_800AD638 = (u8)(*(u8 *)&D_800AD638 % 3U);
        } else {
            *(u8 *)&D_800AD638 = (u8)(*(u8 *)&D_800AD638 & 1U);
        }

        pad_trig = SCRATCHPAD_U16(0x234);
        row = *(u8 *)&D_800AD638;
        if (pad_trig & 0x8000) {
            func_800AB620(0x50D);
            if (row == 0) {
                D_8010A5A8[1] += 2;
            } else if (row == 1) {
                D_8010A5A8[0] += 5;
            } else if (row == 2) {
                opt_val = SCRATCHPAD_U8(0x2E2);
                if (opt_val == 2) {
                    SCRATCHPAD_U8(0x2E2) = 0;
                } else if (opt_val == 0) {
                    SCRATCHPAD_U8(0x2E2) = 3;
                } else if (opt_val == 3) {
                    SCRATCHPAD_U8(0x2E2) = 2;
                }
            }
        } else if (pad_trig & 0x2000) {
            func_800AB620(0x50D);
            if (row == 0) {
                D_8010A5A8[1] += 1;
            } else if (row == 1) {
                D_8010A5A8[0] += 1;
            } else if (row == 2) {
                opt_val = SCRATCHPAD_U8(0x2E2);
                if (opt_val == 2) {
                    SCRATCHPAD_U8(0x2E2) = 3;
                } else if (opt_val == 0) {
                    SCRATCHPAD_U8(0x2E2) = 2;
                } else if (opt_val == 3) {
                    SCRATCHPAD_U8(0x2E2) = 0;
                }
            }
        }

        D_8010A5A8[1] = (u8)(D_8010A5A8[1] % 3U);
        D_8010A5A8[0] = (u8)(D_8010A5A8[0] % 6U);

        D_801D7D80[D_801D7D56] = D_8010A5A8[1];
        D_801D7D78[D_801D7D56] = D_8010A5A8[0];
        D_801D7D68[D_801D7D56] = SCRATCHPAD_U8(0x2E2);

        D_80105508[2].half18 = (u16)(*(u8 *)&D_800AD638 * 14);
        func_80193990();
        break;
    }

    case 0x10:
        func_801942E0(0, D_8011D720, (u16)(s16)-0xA2, 0x37, 0x14E, 0, 0, 2, 0, 3, 3, 1);
        D_8010A322++;
        break;

    case 0x11:
        if (func_80193884() != 0) {
            break;
        }
        if (func_80191E44() != 0) {
            func_80193990();
            D_8010A322 = 2;
            break;
        }
        if (SCRATCHPAD_U16(0x234) & 0x800) {
            func_800AB620(0x528);
            D_801D7D44[1] = 1;
            D_8010A322++;
        }
        break;

    case 0x12:
        D_801D1B11 = (u8)(D_801D7D56 + '0');
        while (func_800ACEC8(D_801D1B00, D_801DA2C8) == 0) {
        }
        func_801929B4(0);
        func_801942E0(0, D_8011D724, (u16)(s16)-0xA2, 0x37, 0x14E, 0, 0, 2, 0, 3, 3, 1);
        D_801D5A48 = 0;
        D_8010A322++;
        break;

    case 0x13:
        D_801D7D44[1] = 1;
        D_801D5A48++;
        res = func_80192A30(0, 0, D_801D1B00, D_801D1AFC, 0x1E80, 1);
        D_801D7D4C = res;

        /* jtbl_801893A8 indexed by (res + 4) for 0..5 */
        switch (res) {
        case -4:
        case -1:
            D_8010A322 = 0x90;
            break;
        case -3:
            D_8010A322 = 0x70;
            break;
        case -2:
            D_8010A322 = 0x60;
            break;
        case 0:
            if (D_8010A322 != 0x13) {
                D_801D5A48 = 0;
            }
            break;
        case 1:
            if (SCRATCHPAD_U8(0x29A) == 0x22) {
                func_801B8708(1);
            }
            func_80192174();
            if (SCRATCHPAD_U8(0x2E1) == 6) {
                D_8010A322 = 3;
            } else {
                D_8010A322 = 0xF0;
            }
            break;
        default:
            D_801D7D40 = 2;
            D_8010A322 = 0xD0;
            break;
        }
        break;

    case 0x20:
        if (D_801D8C68 == 15) {
            D_8010A322 = 0x80;
            break;
        }
        func_801942E0(0, D_8011D638, (u16)(s16)-0xA2, 0x37, 0x14E, 0, 0, 2, 0, 3, 3, 1);
        D_8010A322++;
        break;

    case 0x21:
        if (func_80193884() != 0) {
            break;
        }
        if (func_80191E44() != 0) {
            func_80193990();
            D_8010A322 = 2;
            break;
        }
        if (SCRATCHPAD_U16(0x234) & 0x800) {
            func_800AB620(0x528);
            func_80192594(0);
            func_80192428();
            func_80191E80();
            func_801929B4(0);
            if (D_801D7D54 != 0) {
                D_801D7D44[1] = 3;
                D_8010A322 = 0x70;
            } else {
                func_801942E0(0, D_8011D63C, (u16)(s16)-0xA2, 0x37, 0x14E, 0, 0, 2, 0, 3, 3, 1);
                D_801D5A48 = 0;
                D_801D7D44[1] = 2;
                D_801D1B1A = 0;
                D_8010A322++;
            }
        }
        break;

    case 0x22:
        D_801D5A48++;
        res = func_801930F4(0, 0, 1, D_801D1B00, D_801D1AFC, 0x1E80);

        /* jtbl_801893C0 indexed by (res + 4) for 0..5 */
        switch (res) {
        case -4:
        case -1:
            D_8010A322 = 0x90;
            break;
        case -3:
            D_8010A322 = 0x70;
            break;
        case -2:
            D_8010A322 = 0x60;
            break;
        case 0:
            break;
        case 1:
        default:
            if (func_80193884() == 0) {
                func_801929B4(0);
                D_8010A322++;
            }
            break;
        }
        if (D_8010A322 != 0x22 && D_8010A322 != 0x23) {
            D_801D5A48 = 0;
        }
        break;

    case 0x23:
        D_801D5A48++;
        res = func_80192A30(0, 0, D_801D1B00, D_801D1AFC, 0x1E80, 1);

        /* jtbl_801893D8 indexed by (res + 4) for 0..5 */
        switch (res) {
        case -4:
        case -1:
            D_8010A322 = 0x90;
            break;
        case -3:
            D_8010A322 = 0x70;
            break;
        case -2:
            D_8010A322 = 0x60;
            break;
        case 0:
            break;
        case 1:
        default:
            D_8010A322++;
            break;
        }
        if (D_8010A322 != 0x23) {
            D_801D5A48 = 0;
        }
        break;

    case 0x24:
        if (func_8006F0DC(0x10, 0) != 0) {
            D_801D7D40 = 2;
            SCRATCHPAD_U16(0x294) = 0;
            D_8010A322 = 0xD0;
            if (SCRATCHPAD_U8(0x2E1) != 6) {
                D_8010A322 = 0xE0;
            }
        }
        break;

    case 0x30: {
        u8 *msg = (SCRATCHPAD_U8(0x2E1) == 6) ? D_801D595C : D_8011D640;
        func_801942E0(0, msg, (u16)(s16)-0xA2, 0x37, 0x14E, 0, 0, 2, 0, 3, 3, 1);
        D_801D7D44[1] = 2;
        D_8010A322++;
        break;
    }

    case 0x31:
        if (func_80193884() != 0) {
            break;
        }
        if (func_80191E44() != 0) {
            if (SCRATCHPAD_U8(0x2E1) == 6) {
                D_8010A322 = 4;
                func_8001CEE0(2, 0x5008, 0x50000000U, 0, 0, 0x17, 0, 1);
                D_80105508[2].word0 = 3;
                D_80105508[2].half18 = (u16)(*(u8 *)&D_800AD638 * 14);
                func_80193990();
            } else {
                func_80193990();
                D_8010A322 = 2;
            }
            break;
        }
        if (SCRATCHPAD_U16(0x234) & 0x800) {
            func_800AB620(0x528);
            func_80192594(0);
            D_801D1B1A = 1;
            func_801929B4(0);
            func_801942E0(0, D_8011D63C, (u16)(s16)-0xA2, 0x37, 0x14E, 0, 0, 2, 0, 3, 3, 1);
            func_80192428();
            func_80191E80();
            D_8010A322 = 0x22;
            D_801D7D44[1] = 2;
            D_801D5A48 = 0;
        }
        break;

    case 0x40:
        func_801942E0(0, D_8011D644, (u16)(s16)-0xA2, 0x37, 0x14E, 0, 0, 2, 0, 3, 3, 1);
        D_8010A322++;
        break;

    case 0x41:
        if (func_80193884() != 0) {
            break;
        }
        if (func_80191E44() != 0) {
            func_80193990();
            D_8010A322 = 2;
            break;
        }
        if (SCRATCHPAD_U16(0x234) & 0x800) {
            func_80192594(0);
            func_801942E0(0, D_8011D648, (u16)(s16)-0xA2, 0x37, 0x14E, 0, 0, 2, 0, 3, 3, 1);
            func_800AB620(0x528);
            SCRATCHPAD_U16(0x294) = 0xE;
            D_8010A322++;
        }
        break;

    case 0x42: {
        u16 delay = SCRATCHPAD_U16(0x294);
        if (delay != 0) {
            SCRATCHPAD_U16(0x294) = delay - 1;
            break;
        }
        D_801D7D44[1] = 6;
        D_801D1B24[0] = 0;
        D_8010A322 = 0x99;
        break;
    }

    case 0x43:
    case 0x73:
        if (func_8006F0DC(0x10, 0) != 0) {
            D_801D7D40 = 2;
            SCRATCHPAD_U16(0x294) = 0;
            D_8010A322 = 0xD0;
        }
        break;

    case 0x50:
        D_80105508[1].byte4 = 0;
        D_80105508[3].byte4 = 0;
        func_801942E0(0, D_8011D614, (u16)(s16)-0xA2, 0x37, 0x14E, 0, 0, 2, 0, 3, 3, 1);
        func_801929B4(0);
        D_8010A322++;
        break;

    case 0x51:
        if (func_80193884() != 0) {
            break;
        }
        if (func_8006F07C(0x10) == 0) {
            break;
        }
        if (SCRATCHPAD_U16(0x234) & 0xFFF) {
            func_800AB620(0x529);
            D_8010A322 = 0xE0;
        }
        break;

    case 0x60:
        D_801D7D56 = 0;
        D_80108666 = 0;
        func_80193FB4(8, 0x500A, 0, 0, 0, 0, 0x1C, 0x1000, 0x1000,
                      0, 0, 0x80, 0x80, 0x80, 0, 3, 0);
        func_80193FB4(9, 0x500B, 0, 0, 0, 0, 0x1C, 0x1000, 0x1000,
                      0, 0, 0x80, 0x80, 0x80, 0, 3, 0);
        D_80105508[1].byte4 = 0;
        D_80105508[3].byte4 = 0;
        func_801942E0(0, D_8011D618, (u16)(s16)-0xA2, 0x37, 0x14E, 0, 0, 2, 0, 3, 3, 1);
        D_8010A322++;
        break;

    case 0x61:
        if (func_8006F07C(0x10) == 0) {
            break;
        }
        if (func_80191E44() != 0) {
            D_8010A322 = 0xE0;
            break;
        }
        if (SCRATCHPAD_U16(0x234) & 0x20) {
            func_800AB620(0x528);
            D_801D1B20 = 0;
            D_8010A322++;
            func_801929B4(0);
        }
        break;

    case 0x62:
        if (func_8006F0DC(0x10, 0) != 0) {
            D_8010A322 = 0;
        }
        break;

    case 0x63: {
        u16 delay = SCRATCHPAD_U16(0x294);
        if (delay != 0) {
            SCRATCHPAD_U16(0x294) = delay - 1;
            break;
        }
        D_801D7D40 = 2;
        D_8010A322 = 0xD0;
        D_801D7D44[0] = 0;
        break;
    }

    case 0x70:
        func_80193FB4(8, 0x500A, 0, 0, 0, 0, 0x1C, 0x1000, 0x1000,
                      0, 0, 0x80, 0x80, 0x80, 0, 3, 0);
        func_80193FB4(9, 0x500B, 0, 0, 0, 0, 0x1C, 0x1000, 0x1000,
                      0, 0, 0x80, 0x80, 0x80, 0, 3, 0);
        D_80105508[3].byte4 = 0;
        func_80192594(0);
        while (func_80192720(0) == 0) {
        }
        func_801942E0(0, D_8011D64C, (u16)(s16)-0xA2, 0x37, 0x14E, 0, 0, 2, 0, 3, 3, 1);
        D_8010A322++;
        break;

    case 0x71:
        if (func_80193884() != 0) {
            break;
        }
        if (func_8006F07C(0x10) == 0) {
            break;
        }
        if (func_80191E44() != 0) {
            D_8010A322 = 0xE0;
            break;
        }
        pad_trig = SCRATCHPAD_U16(0x234);
        if (pad_trig & 0x20) {
            func_800AB620(0x528);
            D_801D1B20 = 0;
            func_801929B4(0);
            D_8010A322 = 0x62;
        } else if (pad_trig & 0x800) {
            func_800AB620(0x528);
            func_801942E0(0, D_8011D634, (u16)(s16)-0xA2, 0x37, 0x14E, 0, 0, 2, 0, 3, 3, 1);
            SCRATCHPAD_U16(0x294) = 8;
            D_8010A322++;
        }
        break;

    case 0x72: {
        u16 delay = SCRATCHPAD_U16(0x294) - 1;
        SCRATCHPAD_U16(0x294) = delay;
        if (delay != 0) {
            break;
        }
        if (func_800C5E4C(0) == 1) {
            func_801942E0(0, D_8011D63C, (u16)(s16)-0xA2, 0x37, 0x14E, 0, 0, 2, 0, 3, 3, 1);
            func_80192428();
            func_80191E80();
            func_801929B4(0);
            D_8010A322 = 0x22;
            D_801D7D44[1] = 2;
            D_801D1B24[1] = 0;
        } else {
            SCRATCHPAD_U16(0x294) = 1;
            D_801D1B24[1]++;
            if (D_801D1B24[1] >= 3) {
                D_801D1B24[1] = 0;
                D_8010A322 = 0x90;
                D_801D7D44[1] = 3;
            }
        }
        break;
    }

    case 0x80:
        D_80105508[1].byte4 = 0;
        D_80105508[3].byte4 = 0;
        func_80192594(0);
        func_801942E0(0, D_8011D650, (u16)(s16)-0xA2, 0x37, 0x14E, 0, 0, 2, 0, 3, 3, 1);
        D_8010A322++;
        break;

    case 0x81:
        if (func_80193884() != 0) {
            break;
        }
        if (func_8006F07C(0x10) == 0) {
            break;
        }
        if (func_80191E44() == 0) {
            break;
        }
        for (i = 0; i < 5; i++) {
            if (D_801D7DA0[i] != 0) {
                i = 9;
            }
        }
        if (i == 5) {
            D_8010A322 = 0xE0;
            break;
        }
        func_800AB620(0x529);
        func_80193990();
        D_80105508[1].byte4 = 1;
        D_8010A322 = 2;
        break;

    case 0x90: {
        u8 sub = D_801D7D44[1];
        if ((u8)(sub - 1) >= 2) {
            D_80105508[1].byte4 = 0;
            D_80105508[3].byte4 = 0;
        }
        func_800AB620(0x512);
        func_801942E0(0, D_801D58C4[0x1C + D_801D7D44[1]],
                      (u16)(s16)-0xA2, 0x37, 0x14E, 0, 0, 2, 0, 3, 3, 1);
        D_8010A322++;
        break;
    }

    case 0x91: {
        u8 sub;

        if (func_80193884() != 0) {
            D_801D7D44[1] = 0;
            break;
        }
        if (func_8006F07C(0x10) == 0) {
            if (D_801D7D44[1] == 0) {
                D_8010A322 = 0x61;
            }
            break;
        }
        if (func_80191E44() != 0) {
            func_800AB620(0x529);
            sub = D_801D7D44[1];
            if (sub == 1) {
                func_801929B4(0);
                D_801D7D40 = 2;
                D_8010A322 = 0xD8;
            } else if (sub == 2) {
                func_80192594(0);
                D_801D1B24[0] = 0x20;
                if (D_801D1B1A != 0) {
                    func_801942E0(0, D_801D5948, (u16)(s16)-0xA2, 0x37, 0x14E,
                                  0, 0, 2, 0, 3, 3, 1);
                }
                D_8010A322 = 0x98;
            } else {
                D_8010A322 = 0xE0;
            }
            D_801D7D44[1] = 0;
            break;
        }
        if (SCRATCHPAD_U16(0x234) & 0x20) {
            func_800AB620(0x528);
            sub = D_801D7D44[1];
            if (sub == 1) {
                func_801942E0(0, D_8011D724, (u16)(s16)-0xA2, 0x37, 0x14E,
                              0, 0, 2, 0, 3, 3, 1);
                D_8010A322 = 0x12;
            } else if (sub == 2) {
                func_80192594(0);
                for (i = 0; i < 0x30; i++) {
                    func_800BA518(0);
                }
                D_801D1B11 = (u8)(D_801D7D56 + '0');
                if (func_800ACEE8(D_801D1B00) != 0) {
                    D_801D1B1A = 0;
                }
                func_801929B4(0);
                func_801942E0(0, D_8011D63C, (u16)(s16)-0xA2, 0x37, 0x14E,
                              0, 0, 2, 0, 3, 3, 1);
                func_80192428();
                func_80191E80();
                D_8010A322 = 0x22;
                D_801D7D44[1] = 2;
            } else if (sub == 3) {
                func_801942E0(0, D_8011D634, (u16)(s16)-0xA2, 0x37, 0x14E,
                              0, 0, 2, 0, 3, 3, 1);
                SCRATCHPAD_U16(0x294) = 8;
                D_8010A322 = 0x72;
            } else {
                D_8010A322 = 0xE0;
            }
        }
        break;
    }

    case 0x98:
        if (D_801D1B24[0] != 0) {
            D_801D1B24[0]--;
            break;
        }
        if (D_801D1B1A == 0) {
            D_8010A322 = 0x9C;
            break;
        }
        func_801942E0(0, D_801D5948, (u16)(s16)-0xA2, 0x37, 0x14E, 0, 0, 2, 0, 3, 3, 1);
        D_801D1B24[0] = 0;
        D_8010A322++;
        break;

    case 0x99:
        D_801D1B24[0]++;
        D_801D1B11 = (u8)(D_801D7D56 + '0');
        if (func_800ACEC8(D_801D1B00, D_801DA2C8) != 0) {
            D_801D1B24[0] = 0;
            D_8010A322++;
        }
        if (D_801D1B24[0] >= 0x3C) {
            D_801D1B24[0] = 0;
            D_8010A322++;
        }
        break;

    case 0x9A:
        D_801D1B24[0]++;
        D_801D1B11 = (u8)(D_801D7D56 + '0');
        if (func_800ACEE8(D_801D1B00) != 0) {
            D_801D1B24[0] = 0;
            D_801D1B1A = 0;
            D_8010A322 += 2;
        }
        if (D_801D1B24[0] >= 8) {
            s32 idx;
            func_800AB620(0x512);
            idx = (D_801D7D44[1] == 6) ? 0x22 : 0x23;
            func_801942E0(0, D_801D58C4[idx], (u16)(s16)-0xA2, 0x37, 0x14E,
                          0, 0, 2, 0, 3, 3, 1);
            D_801D1B24[0] = 0;
            D_8010A322++;
        }
        break;

    case 0x9B:
        if (func_80193884() != 0) {
            break;
        }
        D_8010A5A4 = (s16)func_8006EC80(0);
        if (SCRATCHPAD_U16(0x234) & 0x20) {
            func_800AB620(0x528);
            D_8010A322++;
        }
        break;

    case 0x9C:
        D_801D7D44[1] = 0;
        func_801929B4(0);
        D_801D7D40 = 2;
        D_8010A322 = 0xD8;
        break;

    case 0xD0:
        if (D_801D7D44[0] == 1) {
            func_80192594(0);
            func_801929B4(0);
            SCRATCHPAD_U16(0x294) = 0x30;
            D_8010A322 = 1;
            break;
        }
        func_8018E4F0();
        D_801D8D90.halves[0] = 0;
        D_801D8D90.halves[1] = 0;
        D_801D8D90.halves[2] = 8;
        D_801D8D90.halves[3] = 4;
        D_801D8D90.bytes[0] = 0x20;
        D_801D8D90.bytes[1] = 0x20;
        D_801D8D90.bytes[2] = 0x60;
        func_801941E0(0xA, 0, &D_801D8D90, 0, 1, 1);
        SCRATCHPAD_U8(0x330) = 0;
        D_801D1B19 = 1;
        D_8010A322++;
        break;

    case 0xD1:
        if (D_801D8D90.halves[2] < 0xD0) {
            D_801D8D90.halves[2] += 0x32;
        } else if (D_801D8D90.halves[3] < 0x34) {
            D_801D8D90.halves[3] += 0xC;
        } else {
            func_801942E0(1, D_801D590C, (u16)(s16)-0x9C, (u16)(s16)-8, 0x138,
                          3, 0, 0, 0, 3, 1, 1);
            D_801D7D44[0] = 1;
            D_8010A322++;
        }
        func_801941E0(0xA, 0, &D_801D8D90, 0, 1, 1);
        break;

    case 0xD2:
        if (D_801D7D44[0] == 0) {
            func_80192594(0);
            func_801929B4(0);
            SCRATCHPAD_U16(0x294) = 0x30;
            D_8010A322 = 1;
            break;
        }
        D_801D7D44[0] = 0;
        break;

    case 0xD4:
        D_801D8FB8 = 0;
        if (D_801D7D44[0] != 1) {
            D_8010A322++;
            break;
        }
        if (D_801D8D90.halves[2] >= 9) {
            D_801D8D90.halves[2] -= 0x32;
        } else if (D_801D8D90.halves[3] >= 5) {
            D_801D8D90.halves[3] -= 0xC;
        } else {
            D_801D7D44[0] = 1;
            D_8010A322++;
        }
        func_801941E0(0xA, 0, &D_801D8D90, 0, 1, 1);
        break;

    case 0xD5: {
        u8 *rec_500B;

        D_801D93BB = 0;
        if (D_801D8A74 == 15) {
            for (i = 0; i < 5; i++) {
                u8 *rec_500A = func_8001D004(0x500A);
                u8 col = (D_801D7DA0[i] != 0) ? 0x7C : 0xDE;
                rec_500A[i * 0x18 + 0x16] = col;
                rec_500A[i * 0x18 + 0x0A] = col;
            }
            func_80193FB4(8, 0x500A, 0, 0, 0, 0, 0x1C, 0x1000, 0x1000,
                          0, 0, 0x80, 0x80, 0x80, 0, 3, 1);
        } else {
            func_80193FB4(8, 0x500A, 0, 0, 0, 0, 0x1C, 0x1000, 0x1000,
                          0, 0, 0x80, 0x80, 0x80, 0, 3, 0);
        }

        rec_500B = func_8001D004(0x500B);
        {
            s32 free_blocks = 15 - D_801D8A74;
            rec_500B[0x3] = (u8)(((free_blocks / 10) << 3) + 0x20);
            rec_500B[0xF] = (u8)(((free_blocks % 10) << 3) + 0x20);
            rec_500B[0xA] = (rec_500B[0x3] == 0x20) ? 0x7C : 0x77;
        }
        func_80193FB4(9, 0x500B, 0, 0, 0, 0, 0x1C, 0x1000, 0x1000,
                      0, 0, 0x80, 0x80, 0x80, 0, 3, 1);

        D_801D93BB = 0;
        D_801D1B19 = 0;
        SCRATCHPAD_U8(0x330) = 1;
        D_8010A322 = (u8)D_801D7D40;
        break;
    }

    case 0xD8:
        if (func_8006F0DC(0x10, 0) != 0) {
            if (D_801D7D44[0] == 1) {
                func_80192594(0);
                func_801929B4(0);
                SCRATCHPAD_U16(0x294) = 0x30;
                D_8010A322 = 1;
            } else {
                D_8010A322 = 0xD0;
            }
        }
        break;

    case 0xD9:
        if (func_8006F0DC(0x10, 0) != 0) {
            s16 slot = D_801D7D56;
            D_801D7D80[slot] = D_801D1B1B;
            D_801D7D78[slot] = D_801D1B1C;
            D_801D7D68[slot] = D_801D1B1D;
            func_80193990();
            D_8010A322 = 2;
        }
        break;

    case 0xE0:
        D_801D7D44[0] = 0;
        D_80108666 = (u8)D_801D7D56;
        func_8018E3B4();
        break;

    case 0xF0:
        D_801D7D44[0] = 0;
        D_80108666 = (u8)D_801D7D56;
        func_8018E2C0();
        break;

    default:
        break;
    }
}
