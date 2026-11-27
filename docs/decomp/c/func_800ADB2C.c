/*
 * func_800ADB2C
 *
 * Purpose:
 *   PsyQ DumpTPage-style debug helper. Unpacks a 16-bit PS1 GPU texture page ID
 *   (tpage) into color mode (bits 7..8), semi-transparency rate abr (bits 5..6),
 *   VRAM X coordinate ((tpage & 0x1F) << 6), and VRAM Y coordinate
 *   (((tpage << 4) & 0x100) | ((tpage >> 2) & 0x200)), then logs them via the
 *   callback pointer D_800CEAC0 with format string D_80014D5C.
 *
 * Arguments:
 *   tpage (unsigned int): 16-bit PS1 texture page attribute word (masked 0xFFFF)
 *
 * Return value:
 *   int: return value from D_800CEAC0 callback (if any)
 *
 * Global fields read:
 *   D_80014D5C (u8[]):   format string for tpage dump
 *   D_800CEAC0 (fn ptr): debug print/log callback
 */

extern unsigned char D_80014D5C[];
extern int (*D_800CEAC0)(unsigned char *fmt, unsigned int mode, unsigned int abr, unsigned int x, unsigned int y);

int func_800ADB2C(unsigned int tpage)
{
    unsigned int tp = tpage & 0xFFFFu;
    unsigned int mode = (tp >> 7) & 0x3u;
    unsigned int abr = (tp >> 5) & 0x3u;
    unsigned int tx = (tp << 6) & 0x7C0u;
    unsigned int ty = ((tp << 4) & 0x100u) | ((tp >> 2) & 0x200u);

    return D_800CEAC0(D_80014D5C, mode, abr, tx, ty);
}
