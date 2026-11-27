/*
 * func_801CC13C (overlay: select)
 *
 * Purpose:
 *   Per-frame state-machine dispatcher for a Memory Card load/check dialog flow
 *   in SELECT.BIN, driven by Scratchpad+0x29B (states 0..13 via jtbl_8018DA04):
 *     - State 0:     Initializes dialog globals (Scratchpad+0x2A2 = 0x84,
 *                    D_801D8D8C = 0, D_801D429A = 0x80, D_801D8A28[0..1] = {0, 4}).
 *     - State 1:     Animates dialog box open via func_801C4FE0 and ramps
 *                    D_801D429A down by 8 via func_801C3EBC until <= 0x40.
 *     - State 2:     Draws header/prompt text via func_801942E0 and opens box
 *                    D_801D8DA8 via func_801941E0; sets D_801D8B14 = 1.
 *     - State 3:     Polls slot choice input (Left=1, Right=0 in D_801D8D8C;
 *                    Circle confirms -> state 7, Cross cancels -> state 4).
 *     - State 4:     Begins closing animation (clears D_801D8FB8/D_801D92BF/D_801D8B14,
 *                    sets D_801D8A28[0] = D_801D8A28[1]).
 *     - State 5:     Animates dialog box close via func_801C4FE0 and ramps
 *                    D_801D429A up by 8 via func_801C3EBC until >= 0x80.
 *     - State 6:     Cleanup and exit (func_801C3A6C, clears Scratchpad+0x2A2
 *                    and D_8010A322, sets Scratchpad+0x29B = 3, +0x29A = 0x10).
 *     - State 7:     Displays status/prompt message D_801D5844[D_801D8A28[2]] via
 *                    func_801C52E4; on completion sets D_801D8D8C = 2 and -> state 8.
 *     - State 8:     Polls Yes/No confirmation (Left=3, Right=2 in D_801D8D8C;
 *                    Circle with 2 -> state 4, Circle with 3 -> state 10, Cross -> state 4).
 *     - State 9:     No-op.
 *     - State 10:    Advances state 10 -> 11 and falls through into state 11.
 *     - State 11:    Executes Memory Card operation func_801D0A20(D_801D8A28[3])
 *                    (result stored in D_801D59CC) and dispatches via jtbl_8018DA3C:
 *                    0=wait, 1=success (sets D_8010A5A8[6] = 0, updates
 *                    D_800FF748+0x8F14, message 0x10 or 0x0F), errors map to
 *                    messages 0x16..0x1A / 0x1E (and clear D_800FF748+0x8F1E on -2).
 *     - State 12:    Displays result message D_801D5844[D_801D8A28[2]] via
 *                    func_801C52E4 and advances to state 13.
 *     - State 13:    Waits for any button press (D_8010A5A4 & 0xFFF, plays SFX 0x529
 *                    and sets D_801D8A28[2] = 0x79) or auto-advances when
 *                    D_801D59CC == 1 until D_801D8A28[2] >= 0x79 -> state 4.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Scratchpad (0x1F800000) fields read / written:
 *   +0x29A (u8, W)   - set to 0x10 in state 6
 *   +0x29B (u8, R/W) - current state (0..13; >= 14 is no-op)
 *   +0x2A2 (u8, W)   - set to 0x84 in state 0, cleared to 0 in state 6
 *
 * Globals read / written:
 *   D_800FF748 (u8[])      - base buffer:
 *                              +0x8F14 (u8): cleared/ORed with 3 on state 11 success
 *                              +0x8F1E (u8): D_80108666 cleared on state 11 error -2
 *                              +0xABDA (u8): D_8010A322 cleared in state 6
 *   D_8010A5A4 (u16, W/R)  - polled pad 0 button mask from func_8006EE60(0)
 *   D_8010A5A8 (u8[], W)   - +0x0C (u16) cleared to 0 on state 11 success
 *   D_801D429A (u8, R/W)   - window open/close alpha or step parameter (0x80 <-> 0x40)
 *   D_801D5844 (u32[])     - message string pointer table indexed by D_801D8A28[2]
 *   D_801D58B0 (u32[])     - prompt string pointer table ([0] used in state 2)
 *   D_801D59A8 (u8, R/W)   - selected mode/slot copy (0 or 1)
 *   D_801D59CC (s32, R/W)  - return code from func_801D0A20
 *   D_801D8A28 (u8[4], R/W)- [0]=anim frame, [1]=4 (max frame), [2]=msg idx, [3]=slot
 *   D_801D8B14 (u8, W)     - cursor/choice active flag
 *   D_801D8D8C (u8, R/W)   - dialog cursor selection (0..3)
 *   D_801D8DA8 (Fields20)  - box descriptor populated in state 2
 *   D_801D8FB8 (u8, W)     - cleared to 0 in state 4
 *   D_801D92BF (u8, W)     - cleared to 0 in state 4
 *   D_801D92F7 (u8, W)     - cleared to 0 in states 2 and 5
 */

#define SCRATCHPAD_U8(off) (*(volatile unsigned char *)(0x1F800000U + (off)))

typedef struct {
    unsigned short halves[4];
    unsigned char  bytes[12];
} Fields20;

extern unsigned char  D_800FF748[];
extern unsigned short D_8010A5A4;
extern unsigned char  D_8010A5A8[];
extern unsigned char  D_801D429A;
extern unsigned int   D_801D5844[];
extern unsigned int   D_801D58B0[];
extern unsigned char  D_801D59A8;
extern int            D_801D59CC;
extern unsigned char  D_801D8A28[4];
extern unsigned char  D_801D8B14;
extern unsigned char  D_801D8D8C;
extern Fields20       D_801D8DA8;
extern unsigned char  D_801D8FB8;
extern unsigned char  D_801D92BF;
extern unsigned char  D_801D92F7;

extern unsigned short func_8006EE60(unsigned int pad_mode);
extern void           func_800AB620(int sfx_id);
extern void           func_801941E0(int slot, int a1, Fields20 *box, int a3, int a4, int a5);
extern void           func_801942E0(int slot, unsigned int text_ptr, int x, int y,
                                      int w, int a5, int a6, int a7,
                                      int a8, int a9, int a10, int a11);
extern void           func_801C3A6C(void);
extern void           func_801C3EBC(unsigned int val);
extern void           func_801C4FE0(int a0, int a1, int a2, int a3,
                                      int a4, int a5, int a6, int a7,
                                      int a8, int a9, int a10, int a11,
                                      int a12, int a13, int a14, int a15,
                                      int a16, int a17);
extern int            func_801C52E4(int a0, int a1, int a2, unsigned int msg_ptr,
                                      int y_off, int a5);
extern int            func_801D0A20(unsigned int slot);

void func_801CC13C(void)
{
    volatile unsigned char  *g_8F14 = (volatile unsigned char *)(D_800FF748 + 0x8F14);
    volatile unsigned char  *g_8F1E = (volatile unsigned char *)(D_800FF748 + 0x8F1E);
    volatile unsigned char  *g_ABDA = (volatile unsigned char *)(D_800FF748 + 0xABDA);
    volatile unsigned short *g_A5A8_C = (volatile unsigned short *)(D_8010A5A8 + 0x0C);
    unsigned int state;

    D_8010A5A4 = func_8006EE60(0U);
    state = SCRATCHPAD_U8(0x29B);

    if (state >= 0x0EU) {
        return;
    }

    switch (state) {
    case 0:
        SCRATCHPAD_U8(0x2A2) = 0x84;
        D_801D8D8C = 0;
        D_801D429A = 0x80;
        D_801D8A28[0] = 0;
        D_801D8A28[1] = 4;
        SCRATCHPAD_U8(0x29B) = (unsigned char)(SCRATCHPAD_U8(0x29B) + 1U);
        break;

    case 1:
        func_801C4FE0(3, 0, 0, 0,
                      0xA8, 0x34, -0x78, -0x0E,
                      0x20, 0x20, 0x60, 0x20,
                      0x20, 0x60, (int)D_801D8A28[0], (int)D_801D8A28[1],
                      0, 1);
        D_801D8A28[0] = (unsigned char)(D_801D8A28[0] + 1U);
        D_801D429A = (unsigned char)(D_801D429A - 8U);
        func_801C3EBC(D_801D429A);
        if (D_801D429A < 0x41U) {
            SCRATCHPAD_U8(0x29B) = (unsigned char)(SCRATCHPAD_U8(0x29B) + 1U);
        }
        break;

    case 2:
        D_801D92F7 = 0;
        func_801942E0(0, D_801D58B0[0], -0x9C, 0x3E, 0x138, 0, 0, 0, 0, 3, 1, 1);
        func_801942E0(1, D_801D58B0[0], -0x80, -0x12, 0x100, 3, 0, 5, 0, 0x0C, 1, 1);
        D_801D8DA8.halves[0] = 0;
        D_801D8DA8.halves[1] = 0;
        D_801D8DA8.halves[2] = 0xA8;
        D_801D8DA8.halves[3] = 0x34;
        D_801D8DA8.bytes[0] = 0x20;
        D_801D8DA8.bytes[1] = 0x20;
        D_801D8DA8.bytes[2] = 0x60;
        func_801941E0(1, 0, &D_801D8DA8, 0, 3, 1);
        D_801D8B14 = 1;
        SCRATCHPAD_U8(0x29B) = (unsigned char)(SCRATCHPAD_U8(0x29B) + 1U);
        break;

    case 3:
        switch (D_8010A5A4) {
        case 0x8000: /* Left */
            if (D_801D8D8C != 1U) {
                func_800AB620(0x50D);
            }
            D_801D8D8C = 1;
            break;

        case 0x2000: /* Right */
            if (D_801D8D8C != 0U) {
                func_800AB620(0x50D);
            }
            D_801D8D8C = 0;
            break;

        case 0x0020: /* Circle */
            func_800AB620(0x528);
            D_801D8B14 = 0;
            SCRATCHPAD_U8(0x29B) = 7;
            D_801D8A28[3] = D_801D8D8C;
            D_801D59A8 = D_801D8D8C;
            D_801D8A28[2] = (unsigned char)(D_801D8D8C + 0x1CU);
            break;

        case 0x0040: /* Cross */
            func_800AB620(0x529);
            SCRATCHPAD_U8(0x29B) = (unsigned char)(SCRATCHPAD_U8(0x29B) + 1U);
            break;

        default:
            break;
        }
        break;

    case 4:
        D_801D8FB8 = 0;
        D_801D92BF = 0;
        D_801D8B14 = 0;
        D_801D8A28[0] = D_801D8A28[1];
        func_801C4FE0(3, 0, 0, 0,
                      0xA8, 0x34, -0x78, -0x0E,
                      0x20, 0x20, 0x60, 0x20,
                      0x20, 0x60, (int)D_801D8A28[1], (int)D_801D8A28[1],
                      0, 1);
        SCRATCHPAD_U8(0x29B) = (unsigned char)(SCRATCHPAD_U8(0x29B) + 1U);
        break;

    case 5:
        func_801C4FE0(3, 0, 0, 0,
                      0xA8, 0x34, -0x78, -0x0E,
                      0x20, 0x20, 0x60, 0x20,
                      0x20, 0x60, (int)D_801D8A28[0], (int)D_801D8A28[1],
                      0, 1);
        if (D_801D8A28[0] != 0U) {
            D_801D8A28[0] = (unsigned char)(D_801D8A28[0] - 1U);
        }
        D_801D429A = (unsigned char)(D_801D429A + 8U);
        func_801C3EBC(D_801D429A);
        if (D_801D429A >= 0x80U) {
            D_801D92F7 = 0;
            SCRATCHPAD_U8(0x29B) = (unsigned char)(SCRATCHPAD_U8(0x29B) + 1U);
        }
        break;

    case 6:
        func_801C3A6C();
        SCRATCHPAD_U8(0x2A2) = 0;
        *g_ABDA = 0;
        SCRATCHPAD_U8(0x29B) = 3;
        SCRATCHPAD_U8(0x29A) = 0x10;
        break;

    case 7:
        if ((func_801C52E4(1, 0xA8, 0x34, D_801D5844[D_801D8A28[2]], -0x10, 3) & 0xFF) != 0) {
            D_801D8D8C = 2;
            D_801D8B14 = 1;
            SCRATCHPAD_U8(0x29B) = (unsigned char)(SCRATCHPAD_U8(0x29B) + 1U);
        }
        break;

    case 8:
        switch (D_8010A5A4) {
        case 0x8000: /* Left */
            if (D_801D8D8C != 3U) {
                func_800AB620(0x50D);
            }
            D_801D8D8C = 3;
            break;

        case 0x2000: /* Right */
            if (D_801D8D8C != 2U) {
                func_800AB620(0x50D);
            }
            D_801D8D8C = 2;
            break;

        case 0x0020: /* Circle */
            func_800AB620(0x528);
            SCRATCHPAD_U8(0x29B) = (D_801D8D8C == 2U) ? 4 : 0x0A;
            D_801D8B14 = 0;
            break;

        case 0x0040: /* Cross */
            func_800AB620(0x529);
            SCRATCHPAD_U8(0x29B) = 4;
            break;

        default:
            break;
        }
        break;

    case 9:
        break;

    case 10:
        SCRATCHPAD_U8(0x29B) = (unsigned char)(SCRATCHPAD_U8(0x29B) + 1U);
        /* fallthrough to case 11 */

    case 11: {
        int res = func_801D0A20(D_801D8A28[3]);
        D_801D59CC = res;

        switch (res + 4) {
        case 0: /* res == -4 */
            D_801D8A28[2] = 0x17;
            SCRATCHPAD_U8(0x29B) = (unsigned char)(SCRATCHPAD_U8(0x29B) + 1U);
            break;

        case 1: /* res == -3 */
            D_801D8A28[2] = 0x19;
            SCRATCHPAD_U8(0x29B) = (unsigned char)(SCRATCHPAD_U8(0x29B) + 1U);
            break;

        case 2: /* res == -2 */
            *g_8F1E = 0;
            D_801D8A28[2] = 0x18;
            SCRATCHPAD_U8(0x29B) = (unsigned char)(SCRATCHPAD_U8(0x29B) + 1U);
            break;

        case 3: /* res == -1 */
            D_801D8A28[2] = 0x16;
            SCRATCHPAD_U8(0x29B) = (unsigned char)(SCRATCHPAD_U8(0x29B) + 1U);
            break;

        case 4: /* res == 0: still pending */
            break;

        case 5: /* res == 1: success */
            *g_A5A8_C = 0;
            if (D_801D59A8 == 0U) {
                *g_8F14 = 0;
                D_801D8A28[2] = 0x10;
            } else {
                D_801D8A28[2] = 0x0F;
                *g_8F14 = (unsigned char)(*g_8F14 | 3U);
            }
            SCRATCHPAD_U8(0x29B) = (unsigned char)(SCRATCHPAD_U8(0x29B) + 1U);
            break;

        default: /* res == 2 or out of range */
            D_801D8A28[2] = (D_801D59A8 == 0U) ? 0x1E : 0x1A;
            SCRATCHPAD_U8(0x29B) = (unsigned char)(SCRATCHPAD_U8(0x29B) + 1U);
            break;
        }
        break;
    }

    case 12: {
        unsigned int msg_idx = D_801D8A28[2];
        int y_off;

        if ((msg_idx - 0x19U) < 2U || msg_idx == 0x1EU) {
            y_off = -0x10;
        } else {
            y_off = -0x08;
        }

        if ((func_801C52E4(1, 0xA8, 0x34, D_801D5844[msg_idx], y_off, 2) & 0xFF) != 0) {
            SCRATCHPAD_U8(0x29B) = (unsigned char)(SCRATCHPAD_U8(0x29B) + 1U);
        }
        break;
    }

    case 13:
        if ((D_8010A5A4 & 0x0FFFU) != 0U) {
            func_800AB620(0x529);
            D_801D8A28[2] = 0x79;
        }
        if (D_801D59CC == 1) {
            D_801D8A28[2] = (unsigned char)(D_801D8A28[2] + 1U);
        }
        if (D_801D8A28[2] >= 0x79U) {
            SCRATCHPAD_U8(0x29B) = 4;
        }
        break;
    }
}
