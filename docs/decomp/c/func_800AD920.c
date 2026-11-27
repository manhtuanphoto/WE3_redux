/*
 * func_800AD920
 *
 * Purpose:
 *   Uploads a 256-entry (256x1 halfword, 8-bit) CLUT to PS1 VRAM via
 *   func_800AE7E0 (LoadImage) and returns the encoded 16-bit CLUT ID via
 *   func_800ADB14 (getClut) — PsyQ LoadClut-style helper.
 *
 * Arguments:
 *   clut (void *):      pointer to 256-color palette data in RAM
 *   x    (signed int):  VRAM X destination
 *   y    (signed int):  VRAM Y destination
 *
 * Return value:
 *   unsigned short: 16-bit CLUT identifier from func_800ADB14(x, y)
 */

extern void func_800AE7E0(unsigned short *rect, void *pix);
extern unsigned int func_800ADB14(signed int x, signed int y);

unsigned short func_800AD920(void *clut, signed int x, signed int y)
{
    unsigned short rect[4];

    rect[0] = (unsigned short)x;
    rect[1] = (unsigned short)y;
    rect[2] = 0x100;
    rect[3] = 1;

    func_800AE7E0(rect, clut);
    return (unsigned short)(func_800ADB14(x, y) & 0xFFFFU);
}
