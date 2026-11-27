/*
 * func_80019564
 *
 * Purpose:
 *   Processes a list of 16-byte VRAM upload commands until an unknown command
 *   opcode (not 0, 5, 9, or 10) is encountered, resetting D_800F3564 to 0 at
 *   termination. Handles direct LoadImage transfers (cmd 0), coordinate-tracked
 *   uploads into D_800FF748+0x7FA4/0x7FA6 (cmd 9), decompressed uploads via
 *   func_80019A44 -> D_800E8208 (cmd 10), and TIM pixel + optional CLUT
 *   uploads via func_800B5D58 (cmd 5).
 *
 * Arguments:
 *   cmds (unsigned char *): pointer to array of 16-byte command records
 *   flag (unsigned int):    when non-zero, applies func_800198B4 to CLUT in cmd 5
 *
 * Return value:
 *   void
 *
 * Globals read & written:
 *   D_8010A368 / D_8010A369 (u8, W): cleared to 0 on entry
 *   D_800F3564 (u8, R/W): offset-mode flag (cleared to 0 on exit)
 *   D_8010A3E8 (u32, R): VRAM slot base offset (0xFF disables table write)
 *   D_800E765C / D_800E7668 (u32, R): VRAM X/Y base offsets when D_800F3564 != 0
 *   D_800FF748 + 0x7FA4/0x7FA6 (s16, W): per-slot VRAM Y/X coordinates (cmd 9)
 *   D_800E8208[] (W/R): temporary decompression buffer (cmd 10)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))

extern unsigned char D_800FF748[];
extern unsigned char D_800E8208[];
extern unsigned char D_8010A368;
extern unsigned char D_8010A369;
extern unsigned char D_800F3564;
extern unsigned int  D_8010A3E8;
extern unsigned int  D_800E765C;
extern unsigned int  D_800E7668;

extern void func_80019A44(unsigned int src, unsigned char *dst);
extern void func_800B5D58(unsigned int tim_data, unsigned char *out_info);
extern void func_800198B4(unsigned int clut_ptr, int pixel_count);
extern void func_800AE7E0(unsigned short *rect, unsigned int addr);
extern void func_800AE534(int mode);

void func_80019564(unsigned char *cmds, unsigned int flag) {
    unsigned short rect[4];
    unsigned char tim_info[32]; /* stack +0x10..+0x2F */

    D_8010A369 = 0;
    D_8010A368 = 0;

    for (;;) {
        unsigned char op = B(cmds, 0x00);

        if (op == 9) {
            if (D_800F3564 != 0) {
                if (D_8010A3E8 != 0xFFu) {
                    unsigned int idx = (unsigned int)B(cmds, 0x01) + D_8010A3E8;
                    H(D_800FF748, idx * 4u + 0x7FA6u) = (unsigned short)(H(cmds, 0x02) + D_800E765C);
                    H(D_800FF748, idx * 4u + 0x7FA4u) = (unsigned short)(H(cmds, 0x04) + D_800E7668);
                }
                rect[0] = (unsigned short)(H(cmds, 0x02) + D_800E765C);
                rect[1] = (unsigned short)(H(cmds, 0x04) + D_800E7668);
                rect[2] = H(cmds, 0x06);
                rect[3] = H(cmds, 0x08);
                func_800AE7E0(rect, W(cmds, 0x0C));
            } else {
                unsigned int idx = B(cmds, 0x01);
                H(D_800FF748, idx * 4u + 0x7FA6u) = H(cmds, 0x02);
                H(D_800FF748, idx * 4u + 0x7FA4u) = H(cmds, 0x04);
                rect[0] = H(cmds, 0x02);
                rect[1] = H(cmds, 0x04);
                rect[2] = H(cmds, 0x06);
                rect[3] = H(cmds, 0x08);
                func_800AE7E0(rect, W(cmds, 0x0C));
            }
        } else if (op == 0x0A) {
            func_80019A44(W(cmds, 0x0C), D_800E8208);
            if (D_800F3564 != 0) {
                rect[0] = (unsigned short)(H(cmds, 0x02) + D_800E765C);
                rect[1] = (unsigned short)(H(cmds, 0x04) + D_800E7668);
            } else {
                rect[0] = H(cmds, 0x02);
                rect[1] = H(cmds, 0x04);
            }
            rect[2] = H(cmds, 0x06);
            rect[3] = H(cmds, 0x08);
            func_800AE7E0(rect, (unsigned int)D_800E8208);
        } else if (op == 0) {
            if (D_800F3564 != 0) {
                rect[0] = (unsigned short)(H(cmds, 0x02) + D_800E765C);
                rect[1] = (unsigned short)(H(cmds, 0x04) + D_800E7668);
            } else {
                rect[0] = H(cmds, 0x02);
                rect[1] = H(cmds, 0x04);
            }
            rect[2] = H(cmds, 0x06);
            rect[3] = H(cmds, 0x08);
            func_800AE7E0(rect, W(cmds, 0x0C));
        } else if (op == 5) {
            func_800B5D58(W(cmds, 0x0C) + 4u, tim_info);
            rect[0] = H(tim_info, 0x04);
            rect[1] = H(tim_info, 0x06);
            rect[2] = H(tim_info, 0x08);
            rect[3] = H(tim_info, 0x0A);
            func_800AE7E0(rect, W(tim_info, 0x0C));

            if ((W(tim_info, 0x00) >> 3) & 1u) {
                rect[0] = H(tim_info, 0x10);
                rect[1] = H(tim_info, 0x12);
                rect[2] = H(tim_info, 0x14);
                rect[3] = H(tim_info, 0x16);
                if ((flag & 0xFFu) != 0) {
                    func_800198B4(W(tim_info, 0x18), (int)rect[2] * (int)rect[3]);
                }
                func_800AE7E0(rect, W(tim_info, 0x18));
            }
        } else {
            D_800F3564 = 0;
            break;
        }

        func_800AE534(0);
        cmds += 0x10;
    }
}
