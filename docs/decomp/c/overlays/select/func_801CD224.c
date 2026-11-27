/*
 * func_801CD224 (overlay: select)
 *
 * Purpose:
 *   Memory Card load/check sub-state machine for the `select` overlay, driven by
 *   `D_801D94D0` (u8 states: 0x00, 0x01, 0x10, 0x20, 0x30, 0x40, 0x50, 0x51,
 *   0xE8, 0xF0). Checks memory card slot 0 status (`func_80192848`, `func_801CF6A8`),
 *   polls load/read operations (`func_801CF760` via jump table `jtbl_8018DA98`),
 *   delegates to `func_801CE010(2, 0xFF)` for card management/formatting,
 *   displays two-slot status/error/prompt messages (`func_801942E0`), and
 *   handles Circle (0x20) / Cross (0x40) / Start (0x800) input on prompts.
 *
 * Arguments:
 *   arg0 (u32): mode/abort flags; bit 0 (`arg0 & 1`) causes error/no-space paths
 *               to abort immediately with return value 2 instead of showing prompts.
 *   arg1 (u32): message display slot bitmask (`& 1` draws slot 0 at y=0x3E,
 *               `& 2` draws slot 1 at y=-8).
 *
 * Return value:
 *   s32: 0 while active, 1 on load/operation completion, 2 on cancel/abort/error.
 *
 * Scratchpad (0x1F800000) offsets:
 *   +0x234 (u16, R)  : newly pressed pad 0 buttons (0x20=Circle, 0x40=Cross, 0x800=Start)
 *   +0x294 (u16, R/W): frame delay / timeout counter
 *   +0x29A (u8,  R)  : menu/match sub-mode (0x30/0x31 checked when card has >= 0xE blocks)
 *
 * Key Globals:
 *   D_800FF748 (u8[])    : resident state base (+0x8F14 = D_8010865C, +0x8F1E = D_80108666)
 *   D_8010865C (u8, R/W) : memory card flags (bit 3 |= 8 on error/Start; cleared when ret==1)
 *   D_80108666 (u8, W)   : last/active MC slot status byte (cleared on unformatted/error)
 *   D_8010A5A4 (s16, W)  : last polled pad 0 button state from func_8006EC80(0)
 *   D_8010A5A8 (u8[])    : settings struct (+0xC = D_8010A5B4)
 *   D_8010A5B4 (s16, R/W): screen/mode flag (2 selects alternate messages & func_801CF964;
 *                          cleared when ret==1, set to 1 when ret==2)
 *   D_801D5844 (u8 *, R) : dialog/window message pointer passed to func_801C52E4
 *   D_801D58C4 (u8 *[],R): table of memory card message pointers
 *   D_801D58E4 (u8 *, R) : memory card prompt/status message pointer
 *   D_801D5A48 (s32, R/W): retry/polling frame counter
 *   D_801D5D68 (u8, W)   : sub-flow completion flag (0, 1, or 2)
 *   D_801D8C68 (s32, R/W): memory card block count from func_80192848(0)
 *   D_801D94D0 (u8, R/W) : primary state variable for this state machine
 *   D_801DA055 (u8, W)   : state variable for func_801CE010
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
extern u8  D_8010865C;
extern s16 D_8010A5A4;
extern u8  D_8010A5A8[];
extern s16 D_8010A5B4;
extern u8 *D_801D5844;
extern u8 *D_801D58C4[];
extern u8 *D_801D58E4;
extern s32 D_801D5A48;
extern u8  D_801D5D68;
extern s32 D_801D8C68;
extern u8  D_801D94D0;
extern u8  D_801DA055;

extern u32 func_8006EC80(u32 pad_mode);
extern void func_800AB620(s32 sfx_id);
extern s32 func_80192720(u32 channel);
extern s32 func_80192848(s32 port);
extern void func_801929B4(u32 channel);
extern s32 func_80193884(void);
extern void func_801942E0(u8 slot, u8 *msg, u16 x, u16 y, u16 w,
                          u8 arg5, u8 arg6, u8 arg7, u8 arg8, u8 arg9,
                          u16 arg10, u8 arg11);
extern u32 func_801C52E4(s32 arg0, s32 arg1, s32 arg2, u8 *arg3,
                         s32 arg4, s32 arg5);
extern u32 func_801CE010(u32 arg0, u32 arg1);
extern s32 func_801CF6A8(s32 card_info);
extern s32 func_801CF760(s32 arg0);
extern void func_801CF964(void);

static void draw_mc_msg(u32 slot_mask, u8 *msg)
{
    if (slot_mask & 1U) {
        func_801942E0(0, msg, (u16)(s16)-0x9C, 0x3E, 0x138,
                      0, 0, 0, 0, 3, 0, 1);
    }
    if (slot_mask & 2U) {
        func_801942E0(1, msg, (u16)(s16)-0x9C, (u16)(s16)-8, 0x138,
                      3, 0, 0, 0, 0xC, 0, 1);
    }
}

s32 func_801CD224(u32 arg0, u32 arg1)
{
    s32 ret_val = 0;
    u32 pad_raw;
    u8 state;
    s32 msg_idx;

    pad_raw = func_8006EC80(0);
    D_8010A5A4 = (s16)pad_raw;
    state = D_801D94D0;

    switch (state) {
    case 0x00:
        D_801D5A48 = 0;
        D_801D94D0 = 1;
        /* fall through into state 0x01 */

    case 0x01: {
        s32 status;

        D_801D8C68 = func_80192848(0);
        status = func_801CF6A8(D_801D8C68);

        if (status == -2) {
            /* D_800FF748 + 0x8F1E == D_80108666 */
            D_800FF748[0x8F1E] = 0;
            if (arg0 & 1U) {
                D_801D94D0 = 0;
                ret_val = 2;
            } else {
                msg_idx = (*(s16 *)(D_8010A5A8 + 0xC) == 2) ? 0x18 : 0xD;
                draw_mc_msg(arg1, D_801D58C4[msg_idx]);
                SCRATCHPAD_U16(0x294) = 0;
                D_801D94D0 = 0x30;
            }
        } else if (status < -1) {
            if (status == -3) {
                if (arg0 & 1U) {
                    D_801D94D0 = 0;
                    ret_val = 2;
                } else {
                    /* D_800FF748 + 0x8F14 == D_8010865C */
                    D_800FF748[0x8F14] |= 8U;
                    msg_idx = (*(s16 *)(D_8010A5A8 + 0xC) == 2) ? 4 : 0xE;
                    draw_mc_msg(arg1, D_801D58C4[msg_idx]);
                    D_801D94D0 = 0xF0;
                }
            } else {
                D_801D94D0 = 0;
                ret_val = 2;
            }
        } else if (status == 1) {
            if (D_801D8C68 < 0xE) {
                if (arg0 & 1U) {
                    D_801D94D0 = 0;
                    ret_val = 2;
                } else {
                    D_800FF748[0x8F14] |= 8U;
                    msg_idx = (*(s16 *)(D_8010A5A8 + 0xC) == 2) ? 4 : 0xE;
                    draw_mc_msg(arg1, D_801D58C4[msg_idx]);
                    D_801D94D0 = 0xF0;
                }
            } else {
                if ((u32)(SCRATCHPAD_U8(0x29A) - 0x30U) < 2U) {
                    D_801D94D0 = 0xE8;
                    msg_idx = 0x20;
                } else {
                    msg_idx = 9;
                    D_801DA055 = 0xB0;
                }
                draw_mc_msg(arg1, D_801D58C4[msg_idx]);
                SCRATCHPAD_U16(0x294) = 0;
                func_801929B4(0);
                while (func_80192720(0) == 0) {
                }
            }
        } else if (status == 0x11) {
            draw_mc_msg(arg1, D_801D58C4[0]);
            func_801929B4(0);
            D_801D94D0 = 0x10;
        } else {
            D_801D94D0 = 0;
            ret_val = 2;
        }
        break;
    }

    case 0x10: {
        s32 res;

        D_801D5A48++;
        res = func_801CF760(0);

        /*
         * Dispatches via jump table jtbl_8018DA98 when (u32)(res + 4) < 6
         * (i.e. res in [-4, 1]).
         *
         * UNCLEAR: The exact jtbl_8018DA98 target for each value in [-4, 1] is
         * stored in the overlay .rodata table and not present in the .s file.
         * Observed target blocks within this case are:
         *   - .L801CD770 (success):
         *       D_801D94D0 = 0;
         *       ret_val = 1;
         *   - .L801CD780 (unformatted / prompt):
         *       D_800FF748[0x8F1E] = 0;
         *       draw_mc_msg(arg1, D_801D58E4);
         *       D_801D94D0 = 0x40;
         *   - .L801CD830 (error / default, also entered when res == -4 or (u32)(res + 4) >= 6):
         *       func_800AB620(0x512);
         *       if (arg0 & 1U) {
         *           D_801D94D0 = 0;
         *           ret_val = 2;
         *       } else {
         *           msg_idx = (res == -4) ? 0x24 : 0xA;
         *           draw_mc_msg(arg1, D_801D58C4[msg_idx]);
         *           D_801D94D0 = 0xF0;
         *       }
         *   - .L801CD918 (keep polling if D_801D94D0 == 0x10).
         */
        if ((u32)(res + 4) >= 6U || res == -4) {
            func_800AB620(0x512);
            if (arg0 & 1U) {
                D_801D94D0 = 0;
                ret_val = 2;
            } else {
                msg_idx = (res == -4) ? 0x24 : 0xA;
                draw_mc_msg(arg1, D_801D58C4[msg_idx]);
                D_801D94D0 = 0xF0;
            }
        } else {
            /* UNCLEAR: Exact jump table branch for res in {-3, -2, -1, 0, 1} */
            if (res == 0) {
                /* Wait / keep polling in state 0x10 */
            } else if (res == 1) {
                D_801D94D0 = 0;
                ret_val = 1;
            } else {
                /* Placeholder for remaining jtbl_8018DA98 entries (.L801CD780 / .L801CD830) */
                D_800FF748[0x8F1E] = 0;
                draw_mc_msg(arg1, D_801D58E4);
                D_801D94D0 = 0x40;
            }
        }

        if (D_801D94D0 != 0x10U) {
            D_801D5A48 = 0;
        }
        break;
    }

    case 0x20: {
        u32 sub_ret = func_801CE010(2U, 0xFFU) & 0xFFU;

        if (sub_ret == 2U) {
            D_801D5D68 = 0;
            msg_idx = (D_8010A5B4 == 2) ? 3 : 0xD;
            draw_mc_msg(arg1, D_801D58C4[msg_idx]);
            SCRATCHPAD_U16(0x294) = 0;
            D_801D94D0 = 0x30;
        } else if (sub_ret < 3U) {
            if (sub_ret == 1U) {
                D_801D94D0 = 0;
                D_801D5D68 = 2;
                ret_val = 1;
            }
        } else if (sub_ret == 8U) {
            SCRATCHPAD_U16(0x294) = 0x3C;
            D_801D94D0 = 0x50;
        }
        break;
    }

    case 0x30:
        if ((pad_raw & 0xFFFU) != 0U) {
            func_800AB620(0x528);
            SCRATCHPAD_U16(0x294) = 0x78;
        }
        SCRATCHPAD_U16(0x294)++;
        if (SCRATCHPAD_U16(0x294) >= 0x79U) {
            D_801D94D0 = 0;
            ret_val = 2;
        }
        break;

    case 0x40: {
        u16 pad_trig = SCRATCHPAD_U16(0x234);

        if (pad_trig & 0x20U) {
            func_800AB620(0x528);
            SCRATCHPAD_U16(0x294) = 0x30;
            D_801D94D0 = 0x50;
        } else if (pad_trig & 0x40U) {
            func_800AB620(0x529);
            msg_idx = (D_8010A5B4 == 2) ? 3 : 0xD;
            draw_mc_msg(arg1, D_801D58C4[msg_idx]);
            SCRATCHPAD_U16(0x294) = 0x14;
            D_801D94D0 = 0x30;
        }
        break;
    }

    case 0x50: {
        u16 timer = SCRATCHPAD_U16(0x294);

        if (timer != 0U) {
            SCRATCHPAD_U16(0x294) = (u16)(timer - 1U);
        } else {
            D_801D94D0 = (u8)(state + 1U);
        }
        break;
    }

    case 0x51:
        if ((func_801C52E4(1, 0xD0, 0x34, D_801D5844, -8, 0) & 0xFFU) != 0U) {
            D_801D94D0 = 1;
        }
        break;

    case 0xE8:
        if ((pad_raw & 0xFFFU) != 0U) {
            func_800AB620(0x528);
            SCRATCHPAD_U16(0x294) = 0x78;
        }
        SCRATCHPAD_U16(0x294)++;
        if (SCRATCHPAD_U16(0x294) >= 0x79U) {
            msg_idx = (D_8010A5B4 == 2) ? 3 : 0xD;
            draw_mc_msg(arg1, D_801D58C4[msg_idx]);
            SCRATCHPAD_U16(0x294) = 0x14;
            D_801D94D0 = 0x30;
        }
        break;

    case 0xF0:
        if (func_80193884() != 0) {
            draw_mc_msg(arg1, D_801D58E4);
            D_801D94D0 = 0x40;
        } else {
            u16 pad_trig = SCRATCHPAD_U16(0x234);

            if (pad_trig & 0x20U) {
                func_800AB620(0x528);
                SCRATCHPAD_U16(0x294) = 0x30;
                D_801D94D0 = 0x50;
            } else if (pad_trig & 0x40U) {
                func_800AB620(0x529);
                msg_idx = (D_8010A5B4 == 2) ? 3 : 0xD;
                draw_mc_msg(arg1, D_801D58C4[msg_idx]);
                SCRATCHPAD_U16(0x294) = 0;
                D_801D94D0 = 0x30;
            } else if (pad_trig & 0x800U) {
                func_800AB620(0x528);
                D_801D5D68 = 1;
                D_8010865C |= 8U;
                func_801929B4(0);
                D_801DA055 = 0x10;
                D_801D94D0 = 0x20;
            }
        }
        break;

    default:
        break;
    }

    /*
     * Epilogue cleanup:
     * When D_8010A5B4 == 2, func_801CF964() is called with `addiu $v0, $zero, 1`
     * in its delay slot, immediately followed by `bne $s5, $v0`.
     * UNCLEAR: Whether func_801CF964() preserves $v0 == 1 or returns a value
     * compared against ret_val ($s5).
     */
    if (*(s16 *)(D_8010A5A8 + 0xC) == 2) {
        func_801CF964();
    }

    if (ret_val == 1) {
        *(s16 *)(D_8010A5A8 + 0xC) = 0;
        D_800FF748[0x8F14] = 0;
    } else if (ret_val == 2) {
        *(s16 *)(D_8010A5A8 + 0xC) = 1;
    }

    return ret_val;
}
