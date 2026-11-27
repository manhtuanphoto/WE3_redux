/*
 * func_800AE748
 *
 * Purpose:
 *   PsyQ ClearImage equivalent: validates/logs rectangle `rect` via
 *   func_800AE59C(D_80014F48, rect), packs the GP0 clear-color word
 *   (0x80000000 | (b << 16) | (g << 8) | r), and invokes the GPU command
 *   dispatcher (*(D_800CEABC + 0x08))(*(D_800CEABC + 0x0C), rect, 8, color).
 *
 * Arguments:
 *   rect (void *): pointer to PS1 RECT { s16 x, y, w, h }
 *   r    (unsigned char): red component (0..255)
 *   g    (unsigned char): green component (0..255)
 *   b    (unsigned char): blue component (0..255)
 *
 * Return value:
 *   signed int: return value from (*(D_800CEABC + 0x08))(...)
 *
 * Globals read:
 *   D_80014F48 (char[], R): diagnostic/label string passed to func_800AE59C
 *   D_800CEABC (u32, R): GPU control struct pointer (+0x08 callback, +0x0C context)
 */

extern char D_80014F48[];
extern unsigned int D_800CEABC;

extern void func_800AE59C(char *label, void *rect);

signed int func_800AE748(void *rect, unsigned char r, unsigned char g, unsigned char b)
{
    unsigned int color;

    func_800AE59C(D_80014F48, rect);

    color = 0x80000000U
          | (((unsigned int)b & 0xFFU) << 16)
          | (((unsigned int)g & 0xFFU) << 8)
          | ((unsigned int)r & 0xFFU);

    return (*(signed int (**)(unsigned int, void *, signed int, unsigned int))(D_800CEABC + 0x08))(
        *(unsigned int *)(D_800CEABC + 0x0C),
        rect,
        8,
        color
    );
}
