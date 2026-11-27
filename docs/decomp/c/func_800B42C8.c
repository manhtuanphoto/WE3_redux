/*
 * func_800B42C8
 *
 * Purpose:
 *   Updates DRAWENV/DISPENV resolution, interlace, dither, and RGB24 flags
 *   without performing a full GPU reset, then re-initializes the 3D projection/
 *   viewport matrices via func_800B4330(w, h).
 *
 * Arguments:
 *   w     (unsigned short): display width
 *   h     (unsigned short): display height
 *   flags (unsigned short): bit 0 -> interlace byte (D_800F0FDC+0x0C),
 *                           bit 2 -> D_8010CD80
 *   dith  (unsigned char):  dither byte written to D_800F0F8C+0x02
 *   rgb24 (unsigned short): 24-bit color byte written to D_800F0FDC+0x0D
 *                           (passed on stack at +0x10)
 *
 * Return value:
 *   None (void)
 *
 * Global fields written:
 *   D_800F0F8C (unsigned char[], DRAWENV + 0x0C):
 *     +0x00 (s16) = 0, +0x02 (u8) = dith, +0x03 (u8) = 0, +0x04 (u8) = 0
 *   D_800F0FDC (unsigned char[], DISPENV + 0x04):
 *     +0x00 (u16) = w, +0x02 (u16) = h, +0x0C (u8) = flags & 1, +0x0D (u8) = rgb24
 *   D_8010CD80 (signed short): flags & 4
 */

#define B(p, o) (*(unsigned char *)((unsigned char *)(p) + (o)))
#define H(p, o) (*(unsigned short *)((unsigned char *)(p) + (o)))

extern unsigned char D_800F0F8C[];
extern unsigned char D_800F0FDC[];
extern signed short  D_8010CD80;

extern void func_800B4330(unsigned short width, unsigned short height);

void func_800B42C8(
    unsigned short w,
    unsigned short h,
    unsigned short flags,
    unsigned char dith,
    unsigned short rgb24)
{
    H(D_800F0F8C, 0x00) = 0;
    B(D_800F0F8C, 0x02) = dith;
    B(D_800F0F8C, 0x03) = 0;
    B(D_800F0F8C, 0x04) = 0;

    H(D_800F0FDC, 0x00) = w;
    H(D_800F0FDC, 0x02) = h;
    B(D_800F0FDC, 0x0C) = (unsigned char)(flags & 0x1U);
    D_8010CD80 = (signed short)(flags & 0x4U);
    B(D_800F0FDC, 0x0D) = (unsigned char)rgb24;

    func_800B4330(w & 0xFFFFU, h & 0xFFFFU);
}
