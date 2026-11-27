/*
 * func_801D123C (overlay: select)
 *
 * Purpose:
 *   Top-level Memory Card quick-load / quick-save dialog overlay state machine,
 *   dispatched on `D_801D5D6A[0]` (u8, states 0..9 via `jtbl_8018DB30`).
 *   Initializes memory card polling, opens/animates the two dialog boxes
 *   (`D_801D8D78` and `D_801D8D90` via `func_801941E0`), runs either
 *   `func_801CE010(0, 0xFF)` (save when `(mode & 0xFF) != 0`) or
 *   `func_801CD224(0, 0xFF)` (load when `(mode & 0xFF) == 0`), renders the
 *   result message (`func_801C52E4`), waits for timeout or pad-0 button press,
 *   collapses the dialog boxes, and restores menu/overlay state on exit.
 *
 * Arguments:
 *   mode (u32): operation mode (low byte `== 0`: load via func_801CD224;
 *               `!= 0`: save via func_801CE010)
 *
 * Return value:
 *   s32: 0 while active, 1 when the dialog flow finishes (state 9)
 *
 * Key Scratchpad (0x1F800000) offsets:
 *   +0x29A (u8, W): set to 1 on completion when D_800CA5DC == 0x31
 *   +0x29B (u8, W): set to 3 (if D_801D4299 != 0) or 0 on completion when D_800CA5DC == 0x31
 *   +0x2A2 (u8, W): set to 0x89 in state 1, cleared to 0 in state 9
 *   +0x330 (u8, W): screen/fade flag (cleared to 0 in states 0/1->2, set to 1 in state 9)
 *
 * Key Globals:
 *   D_801D5D6A (u8[2], R/W): [0] = state (0..9), [1] = frame/timeout counter
 *   D_801D5D68 (u8,    R/W): sub-mode / outcome selector (0, 1, or 2)
 *   D_801D8A40 (u8,    R/W): index into D_801D58C4 for completion message
 *   D_801D8D78 (Fields20): outer/main dialog box descriptor
 *   D_801D8D90 (Fields20): inner/message dialog box descriptor
 *   D_800CA5DC (u8,    R/W): saved/caller screen state (0x31 = in-select return)
 *   D_800CA5DD (u8,    R)  : saved Scratchpad+0x29B value restored via func_800171FC
 *   D_800CA5DE (u8,    R)  : saved D_8010A322 value restored in state 9
 *   D_801D4299 (u8,    R/W): special return sub-mode flag
 *   D_8010A322 (u8,    W)  : D_800FF748 + 0xABDA; main menu state restored in state 9
 *   D_8010865B (u8,    R)  : D_800FF748 + 0x8F13; card availability flag
 *   D_8010865C (u8,    R)  : D_800FF748 + 0x8F14; card slot/flags
 *   D_80108666 (u8,    W)  : D_800FF748 + 0x8F1E; cleared on -2 card status
 *   D_8010A5A4 (s16,   W)  : last polled pad 0 input from func_8006EC80(0)
 *   D_8010A5A8 (u8[],  R)  : shared settings struct (+0x0C s16 status checked in state 1)
 */

#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int16_t  s16;
typedef int32_t  s32;

typedef struct {
    u16 halves[4];
    u8  bytes[12];
} Fields20; /* sizeof == 0x14 (20) */

#define SCRATCHPAD_U8(off) (*(volatile u8 *)(0x1F800000U + (off)))

extern u8  D_800CA5DC;
extern u8  D_800CA5DD;
extern u8  D_800CA5DE;

extern u8  D_800FF748[];
#define D_8010865B (*(volatile u8 *)(D_800FF748 + 0x8F13))
#define D_8010865C (*(volatile u8 *)(D_800FF748 + 0x8F14))
#define D_80108666 (*(volatile u8 *)(D_800FF748 + 0x8F1E))
#define D_8010A322 (*(volatile u8 *)(D_800FF748 + 0xABDA))

extern s16 D_8010A5A4;
extern u8  D_8010A5A8[];

extern u8  D_801D4299;
extern u8 *D_801D58C4[];
extern u8  D_801D5D68;
extern u8  D_801D5D6A[];
extern u8  D_801D8A40;
extern Fields20 D_801D8D78;
extern Fields20 D_801D8D90;
extern u8  D_801D8FA0;
extern u8  D_801D8FB8;
extern u8  D_801D92A3;
extern u8  D_801D92BF;
extern u8  D_801D94D0;
extern u8  D_801DA055;

extern void func_800171C4(s32 a0);
extern void func_800171FC(u32 a0);
extern u16  func_8006EC80(u32 mode);
extern void func_800AB620(s32 cmd);

extern void func_8018E538(u32 a0);
extern void func_80192594(u32 channel);
extern void func_80192608(u32 channel);
extern s32  func_80192720(u32 channel);
extern void func_801929B4(u32 channel);
extern void func_801941E0(u8 index, u32 word, Fields20 *source, u32 a3, u16 half0, u8 byte3);
extern void func_801942E0(u8 index, u8 *text_ptr, s16 x, s16 y, u16 w,
                          u8 a5, u8 a6, u8 a7, u8 a8, u8 a9, u16 a10, u8 a11);
extern u32  func_801C52E4(u32 a0, s32 x, s32 y, u8 *text_ptr, s32 a4, s32 a5);
extern u32  func_801CD224(u32 mode, u32 channel_mask);
extern s32  func_801CE010(u32 mode, u32 channel_mask);
extern void func_801CFB84(void);

s32 func_801D123C(u32 mode)
{
    u8 state = D_801D5D6A[0];
    s32 ret = 0;

    if (state >= 0x0AU) {
        return 0;
    }

    /*
     * Note: `jtbl_8018DB30` table bytes are external to this .s file, but the
     * 10 `jlabel` blocks in func_801D123C.s correspond 1-to-1 in sequential
     * order to states 0..9 as verified by each state's D_801D5D6A[0] transitions.
     */
    switch (state) {
    case 0:
        if (D_800CA5DC != 0x31) {
            func_8018E538(0);
            SCRATCHPAD_U8(0x330) = 0;
            func_80192594(0);
            func_801929B4(0);
            D_801D5D6A[0] = (u8)(D_801D5D6A[0] + 1U);
            break;
        }
        if (D_801D4299 == 0) {
            func_80192594(0);
            func_801929B4(0);
        }
        D_801D5D6A[0] = (u8)(D_801D5D6A[0] + 1U);
        /* fall through into state 1 */

    case 1: {
        D_801D5D6A[1] = 0;
        D_801D5D68 = 0;
        SCRATCHPAD_U8(0x2A2) = 0x89;
        D_801D5D6A[0] = (u8)(D_801D5D6A[0] + 1U);

        if ((mode & 0xFFU) != 0U) {
            func_801CFB84();
            if (D_8010865B == 0 || D_8010865C == 0) {
                D_801D5D6A[0] = 9;
            }
        } else {
            s16 s1 = *(s16 *)(D_8010A5A8 + 0x0C);
            if (s1 == 0) {
                D_801D5D6A[0] = 9;
            } else if (s1 == 1) {
                s32 card_res = func_80192720(0);
                if (card_res == -2) {
                    D_80108666 = 0;
                    D_801D5D6A[0] = 9;
                } else if (card_res == -3) {
                    func_80192608(0);
                } else if (card_res < -1) {
                    D_801D5D6A[0] = 9;
                } else if (card_res == 0) {
                    D_801D5D6A[1] = (u8)(D_801D5D6A[1] + 1U);
                    D_801D5D6A[0] = (D_801D5D6A[1] < 0x78U) ? 1 : 9;
                } else if (card_res != s1) {
                    D_801D5D6A[0] = 9;
                }
            }
        }

        if (D_801D5D6A[0] == 2) {
            s32 i;
            D_801DA055 = 0x18;
            D_801D94D0 = 0;
            SCRATCHPAD_U8(0x330) = 0;

            D_801D8D78.halves[0] = 0;
            D_801D8D78.halves[1] = 0x4E;
            D_801D8D78.halves[2] = 8;
            D_801D8D78.halves[3] = 2;
            for (i = 0; i < 4; i++) {
                D_801D8D78.bytes[i * 3 + 0] = 0x20;
                D_801D8D78.bytes[i * 3 + 1] = 0x20;
                D_801D8D78.bytes[i * 3 + 2] = 0x60;
            }
            func_801941E0(0, 0, &D_801D8D78, 0, 3, 1);

            D_801D8D90.halves[0] = 0;
            D_801D8D90.halves[1] = 0;
            D_801D8D90.halves[2] = 8;
            D_801D8D90.halves[3] = 4;
            D_801D8D90.bytes[0] = 0x20;
            D_801D8D90.bytes[1] = 0x20;
            D_801D8D90.bytes[2] = 0x60;
            func_801941E0(1, 0, &D_801D8D90, 0, 3, 1);
        }
        break;
    }

    case 2:
        if (D_801D8D78.halves[2] < 0x140U) {
            D_801D8D78.halves[2] = (u16)(D_801D8D78.halves[2] + 0x4EU);
            D_801D8D90.halves[2] = (u16)(D_801D8D90.halves[2] + 0x32U);
        } else if (D_801D8D78.halves[3] < 0x22U) {
            D_801D8D78.halves[3] = (u16)(D_801D8D78.halves[3] + 8U);
            D_801D8D90.halves[3] = (u16)(D_801D8D90.halves[3] + 0xCU);
        } else {
            s32 msg_idx = ((mode & 0xFFU) != 0U) ? 0x15 : 0x14;
            u8 *msg_ptr = D_801D58C4[msg_idx];

            func_801942E0(0, msg_ptr, -0x9C, 0x3E, 0x138, 0, 0, 0, 0, 3, 0, 1);
            func_801942E0(1, msg_ptr, -0x9C, -0x12, 0x138, 3, 0, 0, 4, 0xC, 0, 1);

            D_801D5D6A[1] = 0;
            D_801D5D6A[0] = (u8)(D_801D5D6A[0] + 1U);
            func_80192594(0);
            func_801929B4(0);
            do {
            } while (func_80192720(0) == 0);
        }
        func_801941E0(0, 0, &D_801D8D78, 0, 3, 1);
        func_801941E0(1, 0, &D_801D8D90, 0, 3, 1);
        break;

    case 3:
        D_801D5D6A[1] = (u8)(D_801D5D6A[1] + 1U);
        if (D_801D5D6A[1] == 0x30U) {
            func_80192594(0);
            func_801929B4(0);
        }
        if (D_801D5D6A[1] >= 0x3DU) {
            D_801D5D6A[1] = 0;
            D_801D5D6A[0] = (u8)(D_801D5D6A[0] + 1U);
        }
        break;

    case 4: {
        u8 res;
        if ((mode & 0xFFU) != 0U) {
            res = (u8)(func_801CE010(0, 0xFF) & 0xFF);
            if (res == 0U) {
                break;
            }
            if (res == 1U) {
                D_801D8A40 = (D_801D5D68 == 2) ? 0x1B : 0x10;
                D_801D5D68 = 0;
                D_801D5D6A[0] = (u8)(D_801D5D6A[0] + 1U);
            } else {
                D_801D5D6A[0] = 7;
            }
        } else {
            res = (u8)(func_801CD224(0, 0xFF) & 0xFF);
            if (res == 0U) {
                break;
            }
            if (res == 1U) {
                D_801D8A40 = (D_801D5D68 == 2) ? 0x16 : 0x01;
                D_801D5D68 = 0;
                D_801D5D6A[0] = (u8)(D_801D5D6A[0] + 1U);
            } else {
                D_801D5D6A[0] = 7;
            }
        }
        break;
    }

    case 5:
        if ((func_801C52E4(1, (s16)D_801D8D90.halves[2], (s16)D_801D8D90.halves[3],
                           D_801D58C4[D_801D8A40], -8, 0) & 0xFFU) != 0U) {
            u8 msg_id = D_801D8A40;
            D_801D5D6A[1] = (msg_id == 0x16 || msg_id == 0x1B) ? 0x78 : 0x50;
            D_801D5D6A[0] = (u8)(D_801D5D6A[0] + 1U);
        }
        break;

    case 6: {
        s16 btn;
        D_801D5D6A[1] = (u8)(D_801D5D6A[1] - 1U);
        btn = (s16)func_8006EC80(0);
        D_8010A5A4 = btn;
        if ((btn & ~0xF000) != 0) {
            func_800AB620(0x528);
            D_801D5D6A[1] = 0;
        }
        if (D_801D5D6A[1] == 0) {
            D_801D5D6A[0] = (u8)(D_801D5D6A[0] + 1U);
        }
        break;
    }

    case 7:
        D_801D8FA0 = 0;
        D_801D8FB8 = 0;
        D_801D5D6A[0] = (u8)(D_801D5D6A[0] + 1U);
        /* fall through into state 8 */

    case 8:
        if (D_801D8D78.halves[2] >= 0x28U) {
            D_801D8D78.halves[2] = (u16)(D_801D8D78.halves[2] - 0x4EU);
            D_801D8D90.halves[2] = (u16)(D_801D8D90.halves[2] - 0x32U);
        } else if (D_801D8D78.halves[3] >= 5U) {
            D_801D8D78.halves[3] = (u16)(D_801D8D78.halves[3] - 8U);
            D_801D8D90.halves[3] = (u16)(D_801D8D90.halves[3] - 0xCU);
        } else {
            D_801D5D6A[0] = (u8)(D_801D5D6A[0] + 1U);
        }
        func_801941E0(0, 0, &D_801D8D78, 0, 3, 1);
        func_801941E0(1, 0, &D_801D8D90, 0, 3, 1);
        break;

    case 9:
        SCRATCHPAD_U8(0x2A2) = 0;
        D_801D92A3 = 0;
        D_801D92BF = 0;
        D_801D5D6A[1] = 0;
        D_801D5D6A[0] = 0;

        if (D_800CA5DC == 0x31) {
            D_8010A322 = 0;
            SCRATCHPAD_U8(0x29A) = 1;
            if (D_801D4299 != 0) {
                D_801D4299 = 0;
                SCRATCHPAD_U8(0x29B) = 3;
            } else {
                SCRATCHPAD_U8(0x330) = 1;
                SCRATCHPAD_U8(0x29B) = 0;
            }
        } else {
            /*
             * UNCLEAR: At .L801D19DC, $a0 initially holds D_800CA5DC, but the
             * delay slot of `jal func_800171C4` executes `lui $a0, %hi(D_800CA5DD)`,
             * overwriting $a0 with ((uintptr_t)&D_800CA5DD & 0xFFFF0000) before
             * func_800171C4 runs (likely a compiler delay-slot quirk for
             * `func_800171C4(D_800CA5DC)`).
             */
            func_800171C4((s32)((uintptr_t)&D_800CA5DD & 0xFFFF0000U));
            func_800171FC((u32)D_800CA5DD);
            SCRATCHPAD_U8(0x330) = 1;
            D_8010A322 = D_800CA5DE;
        }

        D_800CA5DC = 0x31;
        ret = 1;
        break;
    }

    return ret;
}
