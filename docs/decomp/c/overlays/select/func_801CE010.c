/*
 * func_801CE010 (overlay: select)
 *
 * Purpose:
 *   State machine for Memory Card save / overwrite / format operations in the
 *   `select` overlay, dispatched on `D_801DA055` (u8, states 0x00..0xF0).
 *   Handles card status probing (`func_80192848`, `func_801CF6A8`), existing
 *   save deletion (`func_800ACEC8` / `func_800ACEE8`), async write initiation
 *   and polling (`func_801D08F0`, `func_801CF918`), read-back verification
 *   (`func_801CF760`), card formatting (`func_800C5E4C` with up to 4 retries),
 *   pad-0 prompts (format, overwrite, retry, cancel), and dual-channel dialog
 *   text rendering (`func_801942E0` on channels 0 and 1 per `channel_mask`).
 *
 * Arguments:
 *   mode (u32): caller mode flag (low byte checked: `== 1` returns 3 on cancel
 *               in state 0x80; `!= 0` changes unformatted card prompt in 0x20)
 *   channel_mask (u32): bitmask of UI dialog channels to draw to
 *                       (bit 0 = channel 0, bit 1 = channel 1)
 *
 * Return value:
 *   s32:
 *     0 = operation in progress
 *     1 = save & verification succeeded (clears D_8010865C and D_8010A5B4)
 *     2 = failed / aborted / timed out
 *     3 = user cancelled (when mode==1 in 0x80, or Scratchpad+0x29A==0x13 in 0xB0/0xF0)
 *     8 = user chose Cross in state 0xE0 (switches D_801DA055=0x18, D_801D94D0=0x50)
 *
 * Key Scratchpad (0x1F800000) offsets:
 *   +0x234 (u16, R)  : newly pressed pad 0 buttons
 *                      (0x0020=Cross, 0x0040=Triangle/Back, 0x0800=Start)
 *   +0x294 (u16, R/W): frame delay / message timeout counter
 *   +0x29A (u8,  R)  : match/menu phase (0x13, 0x30, 0x31 alter flow & messages)
 *
 * Key Globals:
 *   D_801DA055 (u8,  R/W): primary state variable (0x00..0xF0)
 *   D_801D94D0 (u8,  W)  : companion load/menu state (set to 0x50 or 0x30 on exit)
 *   D_8010865C (u8,  R/W): D_800FF748 + 0x8F14; card/slot flags (bit 3 = 0x08)
 *   D_80108666 (u8,  W)  : D_800FF748 + 0x8F1E; cleared to 0 on unformatted/no-card
 *   D_8010A5A4 (s16, W)  : last polled pad 0 input from func_8006EC80(0)
 *   D_8010A5A8 (u8[],R/W): shared settings array; +0x0C (D_8010A5B4, s16) read/cleared
 *   D_8010A5B4 (s16, R/W): D_8010A5A8 + 0x0C
 *   D_801D5A48 (s32, R/W): frame counter during async write/verify (0x50/0x51)
 *   D_801D5A50 (s32, R/W): pending format prompt flag (1 = Cross confirms format in 0xB0)
 *   D_801D5A54 (u8,  R/W): card format retry counter (0..3)
 *   D_801D5D68 (u8,  R/W): save sub-mode / completion stage (incremented 1 -> 2 on success)
 *   D_801DADCC (s32, W)  : handle/result from func_801D08F0
 */

#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int16_t  s16;
typedef int32_t  s32;

#define SCRATCHPAD_U8(off)  (*(volatile u8  *)(0x1F800000U + (off)))
#define SCRATCHPAD_U16(off) (*(volatile u16 *)(0x1F800000U + (off)))

extern u8  D_800FF748[];
#define D_8010865C (*(volatile u8 *)(D_800FF748 + 0x8F14))
#define D_80108666_ALIAS (*(volatile u8 *)(D_800FF748 + 0x8F1E))

extern u8  D_80108666;
extern s16 D_8010A5A4;
extern u8  D_8010A5A8[];
extern s16 D_8010A5B4; /* D_8010A5A8 + 0x0C */

extern u8 *D_801D5844;
extern u8 *D_801D58C4[];
extern u8 *D_801D58DC;
extern u8 *D_801D58E0;
extern u8 *D_801D58E4;
extern u8 *D_801D58EC;
extern u8 *D_801D58F4;
extern u8 *D_801D58FC;
extern u8 *D_801D5910;
extern u8 *D_801D5920;
extern u8 *D_801D5964;
extern void *D_801D59E8;

extern s32 D_801D5A48;
extern s32 D_801D5A50;
extern u8  D_801D5A54;
extern u8  D_801D5D68;
extern u8  D_801D94D0;
extern u8  D_801DA055;
extern u8  D_801DA2C8[];
extern s32 D_801DADCC;

extern u16  func_8006EC80(u32 mode);
extern void func_800AB620(s32 cmd);
extern s32  func_800ACEC8(void *pattern, void *dir_entry);
extern s32  func_800ACEE8(void *pattern);
extern s32  func_800C5E4C(u32 channel);

extern s32  func_80192594(u32 channel);
extern s32  func_80192720(u32 channel);
extern s32  func_80192848(s32 port);
extern void func_801929B4(u32 channel);
extern s32  func_80193884(void);
extern void func_801942E0(u8 index, u8 *text_ptr, s16 x, s16 y, u16 w,
                          u8 a5, u8 a6, u8 a7, u8 a8, u8 a9, u16 a10, u8 a11);
extern u32  func_801C52E4(u32 a0, s32 x, s32 y, u8 *text_ptr, s32 a4, s32 a5);
extern s32  func_801CF6A8(s32 used_blocks);
extern s32  func_801CF760(s32 a0);
extern s32  func_801CF918(void);
extern s32  func_801D08F0(u32 flags);

static void draw_dialog_pair(u32 channel_mask, u8 *text_ptr, s16 ch1_y, u8 ch0_flag)
{
    if ((channel_mask & 1U) != 0U) {
        func_801942E0(0, text_ptr, -0x9C, 0x3E, 0x138, 0, 0, 0, 0, 3, 0, ch0_flag);
    }
    if ((channel_mask & 2U) != 0U) {
        func_801942E0(1, text_ptr, -0x9C, ch1_y, 0x138, 3, 0, 0, 0, 0xC, 0, ch0_flag);
    }
}

s32 func_801CE010(u32 mode, u32 channel_mask)
{
    s32 ret = 0;
    u16 pad0_trig = func_8006EC80(0);
    u8 state = D_801DA055;
    s32 used_blocks;
    s32 status;
    s32 msg_idx;
    u16 buttons;

    D_8010A5A4 = (s16)pad0_trig;

    switch (state) {
    case 0x00:
        D_801DA055 = 0x10;
        /* fall through into state 0x10 */
    case 0x10:
        D_801D5A50 = 0;
        if (D_8010865C == 0) {
            D_801DA055 = 0;
            ret = 2;
            break;
        }
        SCRATCHPAD_U16(0x294) = 0x3C;
        D_801DA055 = (u8)(D_801DA055 + 1U);
        break;

    case 0x11:
        if ((func_801C52E4(1, 0xD0, 0x34, D_801D5844, -8, 0) & 0xFFU) != 0U) {
            D_801DA055 = 0x20;
        }
        SCRATCHPAD_U16(0x294) = (u16)(SCRATCHPAD_U16(0x294) - 1U);
        break;

    case 0x18:
        SCRATCHPAD_U16(0x294) = 0x30;
        D_801DA055 = 0x20;
        break;

    case 0x20:
        if (SCRATCHPAD_U16(0x294) != 0U) {
            SCRATCHPAD_U16(0x294) = (u16)(SCRATCHPAD_U16(0x294) - 1U);
            break;
        }

        used_blocks = func_80192848(0);
        status = func_801CF6A8(used_blocks);

        if (status == -2) {
            D_80108666 = 0;
            if ((mode & 0xFFU) != 0U) {
                msg_idx = 8;
                D_801DA055 = 0xB0;
            } else {
                msg_idx = 0xF;
                D_801DA055 = 0xA0;
            }
            draw_dialog_pair(channel_mask, D_801D58C4[msg_idx], -8, 1);
            SCRATCHPAD_U16(0x294) = 0;
        } else if (status < -1) {
            if (status == -3) {
                draw_dialog_pair(channel_mask, D_801D58DC, -8, 1);
                D_801DA055 = 0x70;
            } else {
                D_801DA055 = 0;
                ret = 2;
            }
        } else if (status == 1) {
            if (used_blocks < 0xE) {
                D_8010865C |= 0x8;
                if (SCRATCHPAD_U8(0x29A) == 0x30) {
                    D_801DA055 = 0x40;
                } else {
                    draw_dialog_pair(channel_mask, D_801D58FC, -8, 1);
                    D_801DA055 = 0xF0;
                }
            } else {
                if ((u8)(SCRATCHPAD_U8(0x29A) - 0x30U) < 2U) {
                    msg_idx = 0x20;
                    D_801DA055 = 0xE8;
                } else {
                    msg_idx = 9;
                    D_801DA055 = 0xB0;
                }
                draw_dialog_pair(channel_mask, D_801D58C4[msg_idx], -8, 1);
                SCRATCHPAD_U16(0x294) = 0;
            }
        } else if (status == 0x11) {
            if (SCRATCHPAD_U8(0x29A) != 0x30) {
                D_801DA055 = 0x30;
            } else {
                draw_dialog_pair(channel_mask, D_801D5910, -0x10, 1);
                D_801DA055 = 0xE0;
            }
        } else {
            D_801DA055 = 0;
            ret = 2;
        }
        break;

    case 0x30:
        if (D_8010865C == 0) {
            D_801DA055 = 0;
            ret = 2;
            break;
        }
        if ((D_8010865C & 0x8U) != 0U) {
            while (func_800ACEC8(D_801D59E8, D_801DA2C8) != 0) {
                func_800ACEE8(D_801D59E8);
            }
        }
        D_801DA055 = 0x40;
        break;

    case 0x40:
        msg_idx = (D_801D5D68 == 1) ? 0x1A : 5;
        draw_dialog_pair(channel_mask, D_801D58C4[msg_idx], -8, 1);
        func_801929B4(0);
        D_801DADCC = func_801D08F0(D_8010865C);
        D_801DA055 = 0x50;
        D_801D5A48 = 0;
        break;

    case 0x50:
        D_801D5A48 += 1;
        status = func_801CF918();
        switch (status) {
        case 1:
            func_801929B4(0);
            D_801DA055 = (u8)(D_801DA055 + 1U);
            break;
        case 0:
            break;
        case -2:
            D_80108666_ALIAS = 0;
            draw_dialog_pair(channel_mask, D_801D58E4, -8, 1);
            D_801DA055 = 0xB0;
            break;
        case -3:
            draw_dialog_pair(channel_mask, D_801D58DC, -8, 1);
            D_801DA055 = 0x70;
            break;
        case -1:
        default:
            func_800AB620(0x512);
            draw_dialog_pair(channel_mask, D_801D58EC, -8, 1);
            D_801DA055 = 0xB0;
            break;
        }
        if (D_801DA055 != 0x50 && D_801DA055 != 0x51) {
            D_801D5A48 = 0;
        }
        break;

    case 0x51:
        D_801D5A48 += 1;
        status = func_801CF760(1);
        switch (status) {
        case 1:
            D_801DA055 = 0;
            ret = 1;
            break;
        case 0:
            break;
        case -2:
            D_80108666_ALIAS = 0;
            draw_dialog_pair(channel_mask, D_801D58E4, -8, 1);
            D_801DA055 = 0xB0;
            break;
        case -4:
        case -3:
        case -1:
        default:
            func_800AB620(0x512);
            msg_idx = (status == -4) ? 0x25 : 0x0A;
            draw_dialog_pair(channel_mask, D_801D58C4[msg_idx], -8, 1);
            D_801DA055 = 0xB0;
            break;
        }
        if (D_801DA055 != 0x51) {
            D_801D5A48 = 0;
        }
        break;

    case 0x70:
        func_80192594(0);
        do {
        } while (func_80192720(0) == 0);
        D_801DA055 = 0x80;
        break;

    case 0x80:
        if (func_80193884() != 0) {
            draw_dialog_pair(channel_mask, D_801D58E4, -8, 1);
            D_801DA055 = 0xB0;
            break;
        }
        buttons = SCRATCHPAD_U16(0x234);
        if ((buttons & 0x0800U) != 0U) {
            func_800AB620(0x528);
            draw_dialog_pair(channel_mask, D_801D58E0, -8, 1);
            SCRATCHPAD_U16(0x294) = 8;
            D_801DA055 = 0x90;
        } else if ((buttons & 0x0020U) != 0U) {
            func_800AB620(0x528);
            D_801DA055 = 0x10;
        } else if ((buttons & 0x0040U) != 0U) {
            func_800AB620(0x529);
            if ((mode & 0xFFU) == 1U) {
                return 3;
            }
            msg_idx = (D_801D5D68 == 1) ? 0x17 : 0x0B;
            draw_dialog_pair(channel_mask, D_801D58C4[msg_idx], -8, 1);
            D_801DA055 = 0xA0;
            SCRATCHPAD_U16(0x294) = 0;
        }
        break;

    case 0x90:
        SCRATCHPAD_U16(0x294) = (u16)(SCRATCHPAD_U16(0x294) - 1U);
        if (SCRATCHPAD_U16(0x294) != 0U) {
            break;
        }
        if (func_800C5E4C(0) == 1) {
            D_801DA055 = 0x10;
            break;
        }
        SCRATCHPAD_U16(0x294) = 1;
        D_801D5A54 = (u8)(D_801D5A54 + 1U);
        if (D_801D5A54 < 4U) {
            break;
        }
        D_801D5A54 = 0;
        func_800AB620(0x512);
        draw_dialog_pair(channel_mask, D_801D5964, -8, 1);
        D_801D5A50 = 1;
        D_801DA055 = 0xB0;
        break;

    case 0xA0:
        if ((pad0_trig & 0x0FFFU) != 0U) {
            func_800AB620(0x528);
            SCRATCHPAD_U16(0x294) = 0x78;
        }
        SCRATCHPAD_U16(0x294) = (u16)(SCRATCHPAD_U16(0x294) + 1U);
        if (SCRATCHPAD_U16(0x294) >= 0x79U) {
            D_801DA055 = 0;
            ret = 2;
        }
        break;

    case 0xB0:
        if (func_80193884() != 0) {
            draw_dialog_pair(channel_mask, D_801D58E4, -8, 1);
            D_801D5A50 = 0;
        }
        buttons = SCRATCHPAD_U16(0x234);
        if ((buttons & 0x0020U) != 0U) {
            func_800AB620(0x528);
            if (D_801D5A50 == 1) {
                draw_dialog_pair(channel_mask, D_801D58E0, -8, 1);
                SCRATCHPAD_U16(0x294) = 8;
                D_801D5A50 = 0;
                D_801DA055 = 0x90;
            } else {
                SCRATCHPAD_U16(0x294) = 0x20;
                D_801DA055 = 0xC0;
            }
        } else if ((buttons & 0x0040U) != 0U) {
            func_800AB620(0x529);
            if (SCRATCHPAD_U8(0x29A) == 0x13) {
                return 3;
            }
            if (*(s16 *)(D_8010A5A8 + 0x0C) == 2 || D_801D5D68 == 1) {
                msg_idx = 0x17;
            } else {
                msg_idx = 0x0B;
            }
            draw_dialog_pair(channel_mask, D_801D58C4[msg_idx], -8, 1);
            D_801DA055 = 0xA0;
            SCRATCHPAD_U16(0x294) = 0;
        }
        break;

    case 0xC0:
        if (SCRATCHPAD_U16(0x294) != 0U) {
            SCRATCHPAD_U16(0x294) = (u16)(SCRATCHPAD_U16(0x294) - 1U);
        } else {
            D_801DA055 = 0x10;
        }
        break;

    case 0xE0:
        if (func_80193884() != 0) {
            D_801DA055 = 0xF0;
            break;
        }
        buttons = SCRATCHPAD_U16(0x234);
        if ((buttons & 0x0800U) != 0U) {
            func_800AB620(0x528);
            D_801DA055 = 0x30;
            D_8010865C |= 0x8;
        } else if ((buttons & 0x0020U) != 0U) {
            func_800AB620(0x528);
            D_801DA055 = 0x18;
            D_801D94D0 = 0x50;
            ret = 8;
        } else if ((buttons & 0x0040U) != 0U) {
            func_800AB620(0x529);
            draw_dialog_pair(channel_mask, D_801D5920, -8, 1);
            D_801DA055 = 0xA0;
            SCRATCHPAD_U16(0x294) = 0;
        }
        break;

    case 0xE8:
        if ((pad0_trig & 0x0FFFU) != 0U) {
            func_800AB620(0x528);
            SCRATCHPAD_U16(0x294) = 0x78;
        }
        SCRATCHPAD_U16(0x294) = (u16)(SCRATCHPAD_U16(0x294) + 1U);
        if (SCRATCHPAD_U16(0x294) < 0x79U) {
            break;
        }
        if (SCRATCHPAD_U8(0x29A) == 0x31) {
            msg_idx = 0x0B;
        } else if (D_8010A5B4 == 2) {
            msg_idx = 3;
        } else {
            msg_idx = 0x0D;
        }
        draw_dialog_pair(channel_mask, D_801D58C4[msg_idx], -8, 1);
        SCRATCHPAD_U16(0x294) = 0;
        if (SCRATCHPAD_U8(0x29A) == 0x30) {
            D_801D94D0 = 0x30;
            ret = 2;
        } else {
            D_801DA055 = 0xA0;
        }
        break;

    case 0xF0:
        if (func_80193884() != 0) {
            draw_dialog_pair(channel_mask, D_801D58F4, -8, 1);
            D_801DA055 = 0xB0;
            break;
        }
        buttons = SCRATCHPAD_U16(0x234);
        if ((buttons & 0x0800U) != 0U) {
            func_800AB620(0x528);
            D_801D5D68 = 1;
            D_801DA055 = 0x30;
            D_8010865C |= 0x8;
        } else if ((buttons & 0x0020U) != 0U) {
            func_800AB620(0x528);
            SCRATCHPAD_U16(0x294) = 0x30;
            D_801DA055 = 0xC0;
        } else if ((buttons & 0x0040U) != 0U) {
            func_800AB620(0x529);
            if (SCRATCHPAD_U8(0x29A) == 0x13) {
                return 3;
            }
            msg_idx = (D_801D5D68 == 1) ? 0x17 : 0x0B;
            draw_dialog_pair(channel_mask, D_801D58C4[msg_idx], -8, 1);
            D_801DA055 = 0xA0;
            SCRATCHPAD_U16(0x294) = 0;
        }
        break;

    default:
        break;
    }

    if (ret == 1) {
        D_8010865C = 0;
        *(s16 *)(D_8010A5A8 + 0x0C) = 0;
        if (D_801D5D68 == 1) {
            D_801D5D68 = 2;
        }
    }

    return ret;
}
