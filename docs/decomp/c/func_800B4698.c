/*
 * func_800B4698
 *
 * Purpose:
 *   Updates double-buffered VRAM draw/display origin offsets (PsyQ GsSetOffset-style).
 *   When D_8010CD80 != 0, clears D_800F1070/D_800F1072, sets D_800F0F80+0x00/+0x02
 *   to D_800EF868[0..1] + D_800E7800[buf] / D_800E789C[buf], and calls func_800AEB98.
 *   When D_8010CD80 == 0, computes the opposite buffer's (buf == 0 ? 1 : 0) X/Y
 *   offset, passes (x, y) to func_800B47A8, and records them in D_800F1070/D_800F1072.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Global fields read:
 *   D_8010CD80 (signed short): mode flag (non-zero uses DRAWENV path via func_800AEB98)
 *   D_8010CD54 (signed short): active buffer index (0 or 1)
 *   D_800EF868 (signed short[2]): base screen X, Y offset
 *   D_800E7800 (signed short[2]): per-buffer VRAM X offsets
 *   D_800E789C (signed short[2]): per-buffer VRAM Y offsets
 *
 * Global fields written:
 *   D_800F1070 (signed short), D_800F1072 (signed short): current X, Y offset
 *   D_800F0F80 + 0x00 / +0x02 (unsigned short): updated X, Y when D_8010CD80 != 0
 */

#define H(p, o) (*(unsigned short *)((unsigned char *)(p) + (o)))

extern signed short D_8010CD80;
extern signed short D_8010CD54;
extern signed short D_800EF868[];
extern signed short D_800E7800[];
extern signed short D_800E789C[];
extern signed short D_800F1070;
extern signed short D_800F1072;
extern unsigned char D_800F0F80[];

extern void func_800AEB98(unsigned char *drawenv);
extern void func_800B47A8(int x, int y);

void func_800B4698(void)
{
    signed short buf = D_8010CD54;

    if (D_8010CD80 != 0) {
        D_800F1072 = 0;
        D_800F1070 = 0;
        H(D_800F0F80, 0x00) = (unsigned short)(D_800EF868[0] + D_800E7800[buf]);
        H(D_800F0F80, 0x02) = (unsigned short)(D_800EF868[1] + D_800E789C[buf]);
        func_800AEB98(D_800F0F80 - 8);
    } else {
        int other = (buf == 0) ? 1 : 0;
        int x = (int)D_800EF868[0] + (int)D_800E7800[other];
        int y = (int)D_800EF868[1] + (int)D_800E789C[other];

        func_800B47A8(x, y);
        D_800F1070 = (signed short)x;
        D_800F1072 = (signed short)y;
    }
}
