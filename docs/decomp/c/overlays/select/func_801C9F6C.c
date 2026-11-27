/*
 * func_801C9F6C (overlay: select)
 *
 * Purpose:
 *   Per-frame state-machine dispatcher for a two-side (pad 0 / pad 1) selection
 *   and swap/edit screen in SELECT.BIN, driven by Scratchpad+0x29B (states 0..5
 *   via jtbl_8018D98C):
 *     - State 0: Initializes fade level (D_8010A31E = 0xFF), sets Scratchpad+0x330 = 1,
 *                clears animation counters D_801DA058/D_801DA05C, and advances state.
 *     - State 1: Waits for func_8018F188(1), spawns 16 UI sprite/widget entries
 *                via func_8001CEE0, initializes slot highlights/labels, and advances.
 *     - State 2: Fades in via func_8006F07C(0x10), resets both sides' lock flags
 *                (D_801DA060/D_801DA064) and cursor arrays (D_801D8A14/16/18),
 *                and advances state.
 *     - State 3: Main dual-pad input loop (sides 0 and 1). In unlocked mode
 *                (bit 7 of D_801DA060/D_801DA064 clear), Left/Right toggles
 *                D_801D8A14[side] between 0 and 1 (unless row == 6), Up/Down moves
 *                D_801D8A16[side] (0..6 or 0..5), Circle locks bit 7 (SFX 0x528),
 *                and Cross on side 0 plays SFX 0x529 and advances to state 4.
 *                In locked mode (bit 7 set), if row == 6 runs func_801CACF0(side)
 *                and unlocks; otherwise Up/Down moves secondary row D_801D8A18[side]
 *                (0..5) while blinking the primary row, Circle commits via
 *                func_801CADC4, and Cross cancels/unlocks.
 *     - State 4: Fades out via func_8006F0DC(0x40, 1) and advances state.
 *     - State 5: Waits for func_8018F188(0), clears Scratchpad+0x2A2/+0x330 and
 *                D_8010A322, calls func_800171C4(0x10) and func_800171FC(0).
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Scratchpad (0x1F800000) fields read / written:
 *   +0x29B (u8, R)   - current sub-state (0..5; >= 6 is no-op)
 *   +0x2A2 (u8, W)   - cleared to 0 in state 5
 *   +0x330 (u8, W)   - set to 1 in state 0, cleared to 0 in state 5
 *
 * Globals read / written:
 *   D_800FF748 (u8[])       - base struct buffer:
 *                               +0x5DC0 + slot*40 + 0x10 (s16): cursor X (201 * col)
 *                               +0x5DC0 + slot*40 + 0x12 (s16): cursor Y ((side*45 + row*5)*4)
 *                               +0xABD6 (s16): D_8010A31E fade level
 *                               +0xABDA (u8):  D_8010A322 sub-state byte
 *   D_8010A5A4 (u16, W/R)   - last polled button mask for current pad
 *   D_801DA058 (u32, R/W)   - frame counter (bit 4 drives locked-row blink)
 *   D_801DA05C (u32, R/W)   - cleared in states 0 and 4
 *   D_801DA060 (u32, R/W)   - side 0 flags (bit 7 = 0x80 locked/sub-select mode)
 *   D_801DA064 (u32, R/W)   - side 1 flags (bit 7 = 0x80 locked/sub-select mode)
 *   D_801D8A14 (u8[2], R/W) - per-side column selection (0 or 1)
 *   D_801D8A16 (u8[2], R/W) - per-side primary row cursor (0..6)
 *   D_801D8A18 (u8[2], R/W) - per-side secondary/target row cursor (0..5)
 */

#define SCRATCHPAD_U8(off) (*(volatile unsigned char *)(0x1F800000U + (off)))

extern unsigned char  D_800FF748[];
extern unsigned short D_8010A5A4;
extern unsigned int   D_801DA058;
extern unsigned int   D_801DA05C;
extern unsigned int   D_801DA060;
extern unsigned int   D_801DA064;
extern unsigned char  D_801D8A14[2];
extern unsigned char  D_801D8A16[2];
extern unsigned char  D_801D8A18[2];

extern void           func_8004FD14(unsigned int arg0);
extern void           func_800171DC(void);
extern void           func_800171C4(unsigned int arg0);
extern void           func_800171FC(unsigned int arg0);
extern void           func_8001CEE0(int slot, int res, int a2, int a3,
                                      int a4, int a5, int a6, int a7);
extern unsigned short func_8006EC80(unsigned int pad_mode);
extern int            func_8006F07C(unsigned int step);
extern int            func_8006F0DC(unsigned int step, unsigned int mode);
extern void           func_8006FAF4(int slot, int idx, int val);
extern void           func_800AB620(int sfx_id);
extern int            func_8018E5EC(int cur, int min_val, int max_val);
extern int            func_8018E640(int cur, int min_val, int max_val);
extern void           func_8018EBA4(int slot, int a1, int a2);
extern int            func_8018F188(unsigned int arg0);
extern void           func_801CAAFC(void);
extern void           func_801CACF0(unsigned int side);
extern void           func_801CADC4(unsigned int side, unsigned int col,
                                      unsigned int row_src, unsigned int row_dst);
extern void           func_801CAF78(unsigned int side);
extern void           func_801CB14C(void);
extern void           func_801CB37C(unsigned int side);

void func_801C9F6C(void)
{
    volatile signed short *g_fade_level   = (volatile signed short *)(D_800FF748 + 0xABD6);
    volatile unsigned char *g_substate_22 = (volatile unsigned char *)(D_800FF748 + 0xABDA);
    unsigned int state = SCRATCHPAD_U8(0x29B);

    if (state >= 6U) {
        return;
    }

    switch (state) {
    case 0:
        func_8004FD14(0U);
        *g_fade_level = 0xFF;
        SCRATCHPAD_U8(0x330) = 1;
        func_800171DC();
        D_801DA058 = 0U;
        D_801DA05C = 0U;
        break;

    case 1:
        if (func_8018F188(1U) == 0) {
            return;
        }
        func_8001CEE0(0x0C, 0x8000, 0, 0, 0x00, 0x1C, 0x00, 1);
        func_8001CEE0(0x0D, 0x8002, 0, 0, 0x00, 0x1C, 0x00, 1);
        func_8001CEE0(0x0E, 0x8001, 0, 0, 0x00, 0x1C, 0x00, 1);
        func_8001CEE0(0x0F, 0x8003, 0, 0, 0x00, 0x1C, 0x00, 1);
        func_8001CEE0(0x06, 0x8004, 0, 0, 0x00, 0x1C, 0x00, 1);
        func_8001CEE0(0x07, 0x8005, 0, 0, 0x00, 0x1C, 0x00, 1);
        func_8001CEE0(0x08, 0x8006, 0, 0, 0x00, 0x1C, 0x00, 1);
        func_8001CEE0(0x09, 0x8007, 0, 0, 0x00, 0x1C, 0x00, 1);
        func_8001CEE0(0x0A, 0x800C, 0, 0, 0x00, 0x1C, 0x82, 1);
        func_8001CEE0(0x0B, 0x800C, 0, 0, 0xB4, 0x1C, 0x82, 1);
        func_8001CEE0(0x10, 0x8008, 0, 0, 0x00, 0x1C, 0x00, 1);
        func_8001CEE0(0x11, 0x800A, 0, 0, 0x00, 0x1C, 0x00, 1);
        func_8001CEE0(0x12, 0x8009, 0, 0, 0x00, 0x1C, 0x00, 1);
        func_8001CEE0(0x13, 0x800B, 0, 0, 0x00, 0x1C, 0x00, 1);
        func_8001CEE0(0x14, 0x800D, 0, 0, 0x00, 0x1C, 0x00, 1);
        func_8001CEE0(0x35, 0x800E, 0, 0, 0x00, 0x09, 0x00, 1);
        func_801CAAFC();
        func_8006FAF4(6, 1, 0x8C);
        func_8006FAF4(8, 1, 0x8C);
        func_801CB14C();
        func_801CAF78(0U);
        func_801CAF78(1U);
        func_800171DC();
        break;

    case 2:
        if (func_8006F07C(0x10U) == 0) {
            return;
        }
        D_801DA064 = 0U;
        D_801DA060 = 0U;
        D_801DA058 = 0U;
        D_801D8A14[1] = 0;
        D_801D8A14[0] = 0;
        D_801D8A16[1] = 0;
        D_801D8A16[0] = 0;
        D_801D8A18[1] = 0;
        D_801D8A18[0] = 0;
        func_800171DC();
        break;

    case 3: {
        unsigned int side;

        func_801CB37C(0U);
        func_801CB37C(1U);
        D_801DA058++;

        for (side = 0U; side < 2U; side++) {
            unsigned int flags = (side != 0U) ? D_801DA064 : D_801DA060;
            unsigned char *desc = D_800FF748 + (side + 0x0AU) * 40U;
            unsigned int col;
            unsigned int btn;

            if (((int)flags >> 7) & 1) {
                /* Locked / sub-selection mode */
                unsigned int blink_val;

                col = D_801D8A14[side];
                *(signed short *)(desc + 0x5DD0) = (signed short)(col * 201U);
                *(signed short *)(desc + 0x5DD2) = (signed short)((side * 45U + D_801D8A18[side] * 5U) * 4U);

                func_8006FAF4((int)(side * 2U + col + 6U), (int)D_801D8A18[side] + 1, 0x99);

                blink_val = (((int)D_801DA058 >> 4) & 1) ? 0x99U : 0x8EU;
                func_8006FAF4((int)(side * 2U + D_801D8A14[side] + 6U),
                              (int)D_801D8A16[side] + 1, (int)blink_val);

                if (D_801D8A16[side] == 6U) {
                    func_801CACF0(side);
                    if (side != 0U) {
                        D_801DA064 &= ~0x80U;
                    } else {
                        D_801DA060 &= ~0x80U;
                    }
                    continue;
                }

                btn = func_8006EC80(side);
                D_8010A5A4 = (unsigned short)btn;

                if (btn == 0x1000U || btn == 0x4000U) {
                    func_8006FAF4((int)(side * 2U + D_801D8A14[side] + 6U),
                                  (int)D_801D8A18[side] + 1, 0x8C);
                    func_800AB620(0x50D);
                    D_801D8A18[side] = (unsigned char)func_8018E5EC((int)D_801D8A18[side], 0, 5);
                    btn = D_8010A5A4;
                }

                if (btn == 0x0020U) { /* Circle: confirm swap/action */
                    func_8006FAF4((int)(side * 2U + D_801D8A14[side] + 6U),
                                  (int)D_801D8A16[side] + 1, 0x8C);
                    func_801CADC4(side, D_801D8A14[side], D_801D8A16[side], D_801D8A18[side]);
                    D_801D8A16[side] = D_801D8A18[side];
                    func_8006FAF4((int)(side * 2U + D_801D8A14[side] + 6U),
                                  (int)D_801D8A18[side] + 1, 0x8D);
                    if (side != 0U) {
                        D_801DA064 &= ~0x80U;
                    } else {
                        D_801DA060 &= ~0x80U;
                    }
                    func_800AB620(0x528);
                    btn = D_8010A5A4;
                }

                if (btn == 0x0040U) { /* Cross: cancel sub-selection */
                    if (side != 0U) {
                        D_801DA064 &= ~0x80U;
                    } else {
                        D_801DA060 &= ~0x80U;
                    }
                    func_8006FAF4((int)(side * 2U + D_801D8A14[side] + 6U),
                                  (int)D_801D8A16[side] + 1, 0x8C);
                    D_801D8A16[side] = D_801D8A18[side];
                    func_800AB620(0x529);
                }
            } else {
                /* Unlocked cursor mode */
                col = D_801D8A14[side];
                *(signed short *)(desc + 0x5DD0) = (signed short)(col * 201U);
                *(signed short *)(desc + 0x5DD2) = (signed short)((side * 45U + D_801D8A16[side] * 5U) * 4U);

                func_8006FAF4((int)(side * 2U + col + 6U), (int)D_801D8A16[side] + 1, 0x99);

                btn = func_8006EC80(side);
                D_8010A5A4 = (unsigned short)btn;

                if (btn == 0x8000U || btn == 0x2000U) { /* Left / Right */
                    unsigned int row = D_801D8A16[side];
                    unsigned int next_col;

                    if (row == 6U) {
                        return;
                    }

                    func_8006FAF4((int)(side * 2U + D_801D8A14[side] + 6U), (int)row + 1, 0x8C);
                    func_8018EBA4((int)(side * 2U + D_801D8A14[side] + 0x0CU), 0x8C, 0x8D);
                    func_8018EBA4((int)(side * 2U + D_801D8A14[side] + 0x06U), 0x69, 0x6A);
                    func_8018EBA4((int)(side * 2U + D_801D8A14[side] + 0x06U), 0x8C, 0x8D);
                    func_8018EBA4((int)(side * 2U + D_801D8A14[side] + 0x10U), 0x82, 0x83);
                    func_8018EBA4((int)(side * 2U + D_801D8A14[side] + 0x10U), 0x8C, 0x8D);

                    func_800AB620(0x50D);

                    next_col = (unsigned int)func_8018E640((int)D_801D8A14[side], 0, 1);
                    D_801D8A14[side] = (unsigned char)next_col;

                    func_8018EBA4((int)(side * 2U + (next_col & 0xFFU) + 0x0CU), 0x8D, 0x8C);
                    func_8018EBA4((int)(side * 2U + D_801D8A14[side] + 0x06U), 0x6A, 0x69);
                    func_8018EBA4((int)(side * 2U + D_801D8A14[side] + 0x06U), 0x8D, 0x8C);
                    func_8018EBA4((int)(side * 2U + D_801D8A14[side] + 0x10U), 0x83, 0x82);
                    func_8018EBA4((int)(side * 2U + D_801D8A14[side] + 0x10U), 0x8D, 0x8C);
                    btn = D_8010A5A4;
                }

                if (btn == 0x1000U || btn == 0x4000U) { /* Up / Down */
                    int max_row;
                    unsigned char next_row;

                    func_8006FAF4((int)(side * 2U + D_801D8A14[side] + 6U),
                                  (int)D_801D8A16[side] + 1, 0x8C);
                    func_800AB620(0x50D);
                    max_row = (D_801D8A14[side] == 0U) ? 6 : 5;
                    next_row = (unsigned char)func_8018E5EC((int)D_801D8A16[side], 0, max_row);
                    D_801D8A16[side] = next_row;
                    D_801D8A18[side] = next_row;
                    btn = D_8010A5A4;
                }

                if (btn == 0x0020U) { /* Circle: enter locked mode */
                    if (side != 0U) {
                        D_801DA064 |= 0x80U;
                    } else {
                        D_801DA060 |= 0x80U;
                    }
                    func_800AB620(0x528);
                    btn = D_8010A5A4;
                }

                if (btn == 0x0040U && side == 0U) { /* Cross on side 0: exit screen */
                    func_800AB620(0x529);
                    func_800171DC();
                    return;
                }
            }
        }
        break;
    }

    case 4:
        if (func_8006F0DC(0x40U, 1U) == 0) {
            return;
        }
        D_801DA05C = 0U;
        func_800171DC();
        break;

    case 5:
        if (func_8018F188(0U) == 0) {
            return;
        }
        SCRATCHPAD_U8(0x2A2) = 0;
        SCRATCHPAD_U8(0x330) = 0;
        func_800171C4(0x10U);
        func_800171FC(0U);
        *g_substate_22 = 0;
        break;
    }
}
