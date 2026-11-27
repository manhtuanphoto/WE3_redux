/*
 * func_800AD838 - Upload texture pixels to VRAM and return TPage ID (LoadTPage)
 *
 * Purpose:
 *   Constructs a 4-halfword VRAM RECT from (x, y, vram_w, h) where `vram_w` is
 *   computed from pixel width `w` according to texture bit depth `tp`:
 *     tp == 0 (4-bit):  w / 4
 *     tp == 1 (8-bit):  w / 2
 *     tp == 2 (16-bit): w
 *   Uploads the texture via func_800AE7E0 (LoadImage) and returns the 16-bit
 *   TPage identifier computed by func_800ADAD8(tp, abr, x, y) (GetTPage).
 *
 * Arguments:
 *   pix (void *):       pointer to pixel data in RAM
 *   tp (signed int):    texture color mode (0 = 4bpp, 1 = 8bpp, 2 = 16bpp)
 *   abr (signed int):   semi-transparency mode
 *   x (signed int):     VRAM X destination (pixels)
 *   y (signed int):     VRAM Y destination (lines)
 *   w (signed int):     texture width in pixels
 *   h (signed int):     texture height in lines
 *
 * Return value:
 *   unsigned short: 16-bit TPage ID from func_800ADAD8
 */

extern void func_800AE7E0(unsigned short *rect, void *pix);
extern unsigned int func_800ADAD8(signed int tp, signed int abr, signed int x, signed int y);

unsigned short func_800AD838(
    void *pix,
    signed int tp,
    signed int abr,
    signed int x,
    signed int y,
    signed int w,
    signed int h)
{
    unsigned short rect[4];

    rect[0] = (unsigned short)x;
    rect[1] = (unsigned short)y;
    rect[3] = (unsigned short)h;

    if (tp == 1) {
        rect[2] = (unsigned short)(w / 2);
    } else if (tp < 2) {
        if (tp == 0) {
            rect[2] = (unsigned short)(w / 4);
        }
        /* UNCLEAR: if tp < 0, rect[2] is left uninitialized in original code */
    } else if (tp == 2) {
        rect[2] = (unsigned short)w;
    }
    /* UNCLEAR: if tp > 2, rect[2] is left uninitialized in original code */

    func_800AE7E0(rect, pix);
    return (unsigned short)(func_800ADAD8(tp, abr, x, y) & 0xFFFFu);
}
