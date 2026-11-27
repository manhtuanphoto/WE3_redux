/*
 * func_8018ECB0 (select overlay)
 *
 * Purpose:
 *   Modal confirmation/selection dialog box handler (SELECT.BIN), driven by
 *   D_8010A320 (states 0..10):
 *     - States 0..7: animates dialog box opening from center.
 *     - State 8:     opens full dialog box, text, and cursor highlight sprite.
 *     - State 9:     polls pad 0 (D-pad Up/Down to move cursor D_800AD638,
 *                    Circle 0x20 to confirm, Cross 0x40 to cancel unless disabled
 *                    by D_801D1AC4[dialog_idx]).
 *     - State 10:    cleans up dialog primitives and returns D_801DAE50
 *                    (1 = cancelled, 2 = confirmed), resetting D_8010A320 = 0.
 *
 * Arguments:
 *   dialog_idx (unsigned int, low 8 bits used) - dialog preset index (0..5)
 *
 * Return value:
 *   unsigned int (u8) - 0 while dialog is active; D_801DAE50 (1 or 2) when done
 *
 * Global fields read / written:
 *   D_800AD638 (u16)    - current cursor row index within the dialog
 *   D_800FF748 (u8[])   - +0x5E3C (u8) cleared in state 10 (slot 3 active byte)
 *   D_8010A320 (u8)     - dialog state (0..10; also accessed via D_800FF748+0xABD8)
 *   D_8010A5A4 (u16)    - last polled pad 0 button mask
 *   D_8011D5F4 (u32[])  - dialog text pointer table indexed by D_801D1A98[idx*2]
 *   D_801D1A68..        - dialog layout tables (x, y, w, h; stride 8 bytes)
 *   D_801D1A98, +1      - dialog text ID and style/count bytes (stride 2 bytes)
 *   D_801D1AA4          - option count per dialog (u8)
 *   D_801D1AAC          - cursor Y base offset (s8)
 *   D_801D1AB4          - text X offset (s8)
 *   D_801D1ABC          - text Y offset (s8)
 *   D_801D1AC4          - cancel-disabled flag (u8)
 *   D_801D8FA0 (u8)     - cleared in state 10 for dialog_idx not in {0,1,3,4}
 *   D_801D92DB (u8)     - cleared in state 10 for dialog_idx not in {0,1,3,4}
 *   D_801DAE50 (u8)     - dialog result (1 = cancel, 2 = confirm)
 */

#define B(p, o) (*(unsigned char *)((p) + (o)))

extern unsigned short D_800AD638;
extern unsigned char D_800FF748[];
extern unsigned char D_8010A320;
extern unsigned short D_8010A5A4;
extern unsigned int D_8011D5F4[];
extern signed short D_801D1A68[];
extern unsigned char D_801D1A98[];
extern unsigned char D_801D1AA4[];
extern signed char D_801D1AAC[];
extern signed char D_801D1AB4[];
extern signed char D_801D1ABC[];
extern unsigned char D_801D1AC4[];
extern unsigned char D_801D8FA0;
extern unsigned char D_801D92DB;
extern unsigned char D_801DAE50;

extern unsigned short func_8006EC80(unsigned int pad_mode);
extern void func_8006FCFC(unsigned int clut_idx);
extern void func_800AB620(int cmd);
extern void func_80193FB4(int slot, int a1, int a2, int a3,
                          int x_or_y, int s5, int s6, int s7, int fp,
                          int t0, int t1, int t2, int t3, int a2_2,
                          int a0_2, int a1_2, int flag);
extern void func_801940EC(int slot, int a1, ...);
extern void func_801942E0(int slot, unsigned int text_ptr, int x, int y,
                          int s4, int s5, int s6, int s7, int fp,
                          int t0, int t1, int t2);

unsigned int func_8018ECB0(unsigned int dialog_idx)
{
    unsigned int idx = dialog_idx & 0xFFU;
    signed short cursor_base_y = (signed short)D_801D1AAC[idx];
    signed short text_y = (signed short)D_801D1ABC[idx];
    signed short box_x = D_801D1A68[idx * 4U + 0U];
    signed short box_y = D_801D1A68[idx * 4U + 1U];
    signed short box_w = D_801D1A68[idx * 4U + 2U];
    signed short box_h = D_801D1A68[idx * 4U + 3U];
    int center_y = box_y + (int)((unsigned int)(int)box_h >> 1U);
    unsigned char text_id = D_801D1A98[idx * 2U + 0U];
    unsigned char text_param = D_801D1A98[idx * 2U + 1U];
    unsigned char num_options = D_801D1AA4[idx];
    unsigned char no_cancel = D_801D1AC4[idx];
    int line_step;
    unsigned int state;

    func_8006FCFC(0x36U);

    /* Note: assembly loads 0x18 before the branch `bne idx, 5, .L8018EDB8`,
     * whose delay slot unconditionally sets $fp = 0x16. */
    line_step = 0x16;

    state = D_8010A320;
    if (state >= 0xBU) {
        return 0U;
    }

    switch (state) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7: {
        unsigned int step = state + 1U;
        int cur_y = center_y - (int)(((unsigned int)(int)box_h >> 4U) * step);
        int cur_h = (int)(((unsigned int)(int)box_h >> 3U) * step);

        /* UNCLEAR: In SELECT.BIN, func_801940EC expects a 20-byte struct
         * pointer in $a2, whereas func_8018ECB0 passes 11 scalar arguments
         * (box_x in $a2, etc.). Reproduced verbatim from assembly. */
        func_801940EC(2, 0, (int)box_x, cur_y, (int)box_w, cur_h, 0, 0x30, 0x80, 1, 1);

        D_800AD638 = 0;
        D_8010A320 = (unsigned char)(B(D_800FF748, 0xABD8) + 1U);
        return 0U;
    }

    case 8:
        /* UNCLEAR: Same func_801940EC calling-convention mismatch as cases 0..7. */
        func_801940EC(2, 0, (int)box_x, (int)box_y, (int)box_w, (int)box_h, 0, 0x30, 0x80, 0, 1);

        func_801942E0(0, D_8011D5F4[text_id], (int)D_801D1AB4[idx], (int)text_y,
                      0xB4, 3, 1, 0, 0, (int)text_param, 0, 1);

        func_80193FB4(3, 0x3076, 0, 0,
                      (int)cursor_base_y + line_step * (int)D_800AD638,
                      0x18, 0, 0x1000, 0x1000, 0, 0, 0x80, 0x80, 0x80, 0, 0, 1);

        D_8010A320 = (unsigned char)(B(D_800FF748, 0xABD8) + 1U);
        return 0U;

    case 9: {
        unsigned short btn = func_8006EC80(0U);
        D_8010A5A4 = btn;

        if (btn == 0x1000U || btn == 0x4000U) {
            unsigned short cur;
            unsigned short max_idx;

            func_800AB620(0x50D);
            btn = D_8010A5A4;
            cur = D_800AD638;
            max_idx = (unsigned short)(num_options - 1U);

            if (btn == 0x1000U) {
                cur = (cur != 0U) ? (unsigned short)(cur - 1U) : max_idx;
            } else if (btn == 0x4000U) {
                cur = (cur == max_idx) ? 0U : (unsigned short)(cur + 1U);
            }
            D_800AD638 = cur;
        }

        btn = D_8010A5A4;
        if (btn == 0x20U) {
            func_800AB620(0x528);
            D_801DAE50 = 2;
            D_8010A320 = (unsigned char)(B(D_800FF748, 0xABD8) + 1U);
            return 0U;
        } else if (btn == 0x40U) {
            if (no_cancel != 0U) {
                return 0U;
            }
            func_800AB620(0x529);
            D_801DAE50 = 1;
            D_8010A320 = (unsigned char)(B(D_800FF748, 0xABD8) + 1U);
            return 0U;
        }

        func_80193FB4(3, 0x3076, 0, 0,
                      (int)cursor_base_y + line_step * (int)D_800AD638,
                      0x18, 0, 0x1000, 0x1000, 0, 0, 0x80, 0x80, 0x80, 0, 0, 1);
        return 0U;
    }

    case 10:
        B(D_800FF748, 0x5E3C) = 0;
        if (idx >= 2U && (unsigned int)((idx - 3U) & 0xFFU) >= 2U) {
            D_801D92DB = 0;
            D_801D8FA0 = 0;
        }
        D_8010A320 = 0;
        return D_801DAE50;
    }

    return 0U;
}
