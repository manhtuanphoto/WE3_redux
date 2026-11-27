/*
 * func_801B38E0 (overlay: select)
 *
 * Purpose:
 *   Initializes list header/background widgets via func_80193FB4, then iterates
 *   over D_801DA5D6 visible rows starting at entry index `start_idx` in
 *   D_801D94E8 (45-byte records) to draw each row's text (func_801942E0) and
 *   highlight/backdrop quad (func_801940EC via D_801D8D78). Returns the widget
 *   slot ID of the entry matching D_8010866A.
 *
 * Arguments:
 *   arg0      (int):          unused (overwritten before first call)
 *   start_idx (signed short): starting index in D_801D94E8 table
 *
 * Return value:
 *   unsigned char: widget slot ID (>= 10) for the entry whose ID equals
 *                  D_8010866A (uninitialized on stack if no entry matches)
 *
 * Globals / Structs read & written:
 *   D_801DA5D6 (s16, R): number of visible rows to render
 *   D_801D94E8 + i * 0x2D (45-byte entry):
 *     +0x00 (u8, R): row type (0xFF = header/separator, else normal entry)
 *     +0x02 (u8, R, == D_801D94EA): entry/team ID
 *     +0x03 (char[], R, == D_801D94EB): entry text string
 *   D_800FF748 + 0xAB50 + entry_id (u8, R): status/flag byte (bit 1 tested)
 *   D_801D8F9B + slot * 24 (u8, W): set to 2 when status bit 1 is set
 *   D_801D8D78 (20-byte quad/rect descriptor, W):
 *     +0x00 (s16): x (-200), +0x02 (s16): y (-66 + i*20)
 *     +0x04 (s16): w (410),  +0x06 (s16): h (8 for 0xFF, 10 otherwise)
 *     +0x08..+0x13 (u8[12]): vertex RGB/color bytes
 *   D_8010866A (u8, R): currently selected entry ID
 *
 * Scratchpad (0x1F800000) read:
 *   +0x290 (u32, R): global frame counter (bit 0x10 blinks selected quad)
 *   +0x29B (u8, R):  sub-mode (0 disables highlight check; 0xA skips blink)
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char  D_800FF748[];
extern unsigned char  D_8010866A;
extern unsigned char  D_801D8D78[];
extern unsigned char  D_801D8F9B[];
extern unsigned char  D_801D94E8[];
extern unsigned char  D_801D94EA[];
extern unsigned char  D_801D94EB[];
extern signed short   D_801DA5D6;

extern void func_80193FB4(
    int id, int tex_id, int a2, int a3,
    int s0, int s1, int s2, int s3,
    int s4, int s5, int s6, int r,
    int g, int b, int s10, int s11,
    int s12
);
extern void func_801940EC(
    int slot, unsigned int color_flag, unsigned char *rect,
    int a3, int s0, int s1
);
extern void func_801942E0(
    int slot, unsigned char *str, int x, int y,
    int width, int s1, int s2, int s3,
    int s4, int s5, int s6, int s7
);

unsigned char func_801B38E0(int arg0, signed short start_idx)
{
    int i;
    int slot;
    int y_pos;
    int box_y;
    int row_y_off;
    unsigned char selected_slot; /* UNCLEAR: uninitialized if no row matches D_8010866A */

    (void)arg0;

    func_80193FB4(1, 0x30E0, 0, -0x10, -3, 0x18, 0x63, 0x1000, 0x1000, 0, 0, 0x80, 0x80, 0x80, 0, 0, 1);
    func_80193FB4(2, 0x30AC, 0,  0x00,  0, 0x1D, 0x00, 0x1000, 0x1000, 0, 0, 0x80, 0x80, 0x80, 0, 1, 1);
    func_80193FB4(8, 0x30C0, 0,  0x00,  0, 0x0C, 0x00, 0x1000, 0x1000, 0, 0, 0x80, 0x80, 0x80, 0, 1, 1);

    if (D_801DA5D6 <= 0) {
        return selected_slot;
    }

    i = 0;
    slot = 0x0A;
    y_pos = -0x48;
    box_y = -0x42;
    row_y_off = 0;

    do {
        int rec_off = (start_idx + i) * 0x2D;
        unsigned char row_type = D_801D94E8[rec_off];

        if (row_type == 0xFF) {
            func_801942E0(slot, &D_801D94EB[rec_off], -0xAA, y_pos, 0x154, 2, 2, 0, 0, 2, 4, 1);

            S(D_801D8D78, 0x00) = -0xC8;
            S(D_801D8D78, 0x02) = (signed short)box_y;
            S(D_801D8D78, 0x04) = 0x19A;
            S(D_801D8D78, 0x06) = 8;

            B(D_801D8D78, 0x08) = 0xFF;
            B(D_801D8D78, 0x09) = 0xFF;
            B(D_801D8D78, 0x0A) = 0x80;
            B(D_801D8D78, 0x0B) = 0x00;
            B(D_801D8D78, 0x0C) = 0xFF;
            B(D_801D8D78, 0x0D) = 0xFF;
            B(D_801D8D78, 0x0E) = 0xFF;
            B(D_801D8D78, 0x0F) = 0xFF;
            B(D_801D8D78, 0x10) = 0x80;
            B(D_801D8D78, 0x11) = 0x00;
            B(D_801D8D78, 0x12) = 0xFF;
            B(D_801D8D78, 0x13) = 0xFF;

            func_801940EC(slot, 0x70000000u, D_801D8D78, 2, 5, 1);
        } else {
            unsigned int entry_id;
            unsigned char sub_mode;

            func_801942E0(slot, &D_801D94EB[rec_off], -0xAA, y_pos, 0x154, 1, 0, 0, 0, 1, 4, 1);

            entry_id = D_801D94EA[rec_off];
            if ((B(D_800FF748, 0xAB50 + entry_id) >> 1) & 1u) {
                D_801D8F9B[slot * 24] = 2;
            }

            S(D_801D8D78, 0x00) = -0xC8;
            S(D_801D8D78, 0x02) = (signed short)(row_y_off - 0x42);
            S(D_801D8D78, 0x04) = 0x19A;
            S(D_801D8D78, 0x06) = 0x0A;

            B(D_801D8D78, 0x08) = 0x80;
            B(D_801D8D78, 0x09) = 0x80;
            B(D_801D8D78, 0x0A) = 0x40;
            B(D_801D8D78, 0x0B) = 0x80;
            B(D_801D8D78, 0x0C) = 0x80;
            B(D_801D8D78, 0x0D) = 0x40;
            B(D_801D8D78, 0x0E) = 0x80;
            B(D_801D8D78, 0x0F) = 0x80;
            B(D_801D8D78, 0x10) = 0x40;
            B(D_801D8D78, 0x11) = 0x80;
            B(D_801D8D78, 0x12) = 0x80;
            B(D_801D8D78, 0x13) = 0x40;

            if (entry_id == D_8010866A) {
                selected_slot = (unsigned char)slot;
                sub_mode = B(Scratchpad, 0x29B);
                if (sub_mode != 0 && sub_mode != 0x0A && (W(Scratchpad, 0x290) & 0x10u) != 0) {
                    B(D_801D8D78, 0x08) = 0xA0;
                    B(D_801D8D78, 0x09) = 0xA0;
                    B(D_801D8D78, 0x0A) = 0xA0;
                    B(D_801D8D78, 0x0B) = 0xA0;
                    B(D_801D8D78, 0x0C) = 0xA0;
                    B(D_801D8D78, 0x0D) = 0xA0;
                    B(D_801D8D78, 0x0E) = 0xA0;
                    B(D_801D8D78, 0x0F) = 0xA0;
                    B(D_801D8D78, 0x10) = 0xA0;
                    B(D_801D8D78, 0x11) = 0xA0;
                    B(D_801D8D78, 0x12) = 0xA0;
                    B(D_801D8D78, 0x13) = 0xA0;
                }
            }

            func_801940EC(slot, 0x60000000u, D_801D8D78, 2, 5, 1);
        }

        slot++;
        y_pos += 0x14;
        box_y += 0x14;
        row_y_off += 0x14;
        i++;
    } while (i < D_801DA5D6);

    return selected_slot;
}
