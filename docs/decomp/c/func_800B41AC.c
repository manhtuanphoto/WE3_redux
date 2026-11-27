/*
 * func_800B41AC
 *
 * Purpose:
 *   Graphics/video mode initialization (PsyQ GsInitGraph-style setup).
 *   Resets/configures the GPU via func_800AE1B8, initializes the DRAWENV-like
 *   structure at D_800F0F80 (applying via func_800AEB98), initializes the
 *   DISPENV-like structure at D_800F0FD8 with resolution (w, h), checks
 *   PAL/video mode via func_800BB22C() (setting Y offset 0x18 and PAL flag
 *   when == 1), stores interlace/dither flags and D_8010CD80, and applies
 *   DISPENV via func_800AED64.
 *
 * Arguments:
 *   w (unsigned int): display width (stored as s16 at D_800F0FD8+0x04)
 *   h (unsigned int): display height (stored as s16 at D_800F0FD8+0x06)
 *   flags (unsigned int): bit 0 = +0x10 byte (isinter), bit 2 = D_8010CD80,
 *                         bits 4..5 == 3 -> passes 3 to func_800AE1B8 (else 0)
 *   dith (unsigned int): byte stored at D_800F0F80+0x0E
 *   rgb24 (unsigned short): byte stored at D_800F0FD8+0x11 (isrgb24)
 *
 * Return value:
 *   None (void)
 *
 * Global fields written:
 *   D_800F0F80 (DRAWENV-like struct, passed as &D_800F0F80 - 8 to func_800AEB98)
 *   D_800F0FD8 (DISPENV-like struct, passed to func_800AED64)
 *   D_8010CD80 (signed short): flags & 4
 */

#define B(p, o) (*(unsigned char *)((unsigned char *)(p) + (o)))
#define H(p, o) (*(unsigned short *)((unsigned char *)(p) + (o)))

extern unsigned char D_800F0F80[];
extern unsigned char D_800F0FD8[];
extern signed short D_8010CD80;

extern void func_800AE1B8(unsigned int mode);
extern void func_800AEB98(unsigned char *drawenv);
extern int func_800BB22C(void);
extern void func_800AED64(unsigned char *dispenv);

void func_800B41AC(unsigned int w, unsigned int h, unsigned int flags,
                   unsigned int dith, unsigned short rgb24)
{
    unsigned int reset_mode = 0;

    if (((flags >> 4) & 3u) == 3u) {
        reset_mode = 3u;
    }
    func_800AE1B8(reset_mode);

    H(D_800F0F80, 0x02) = 0;
    H(D_800F0F80, 0x00) = 0;
    H(D_800F0F80, 0x0A) = 0;
    H(D_800F0F80, 0x08) = 0;
    H(D_800F0F80, 0x06) = 0;
    H(D_800F0F80, 0x04) = 0;
    H(D_800F0F80, 0x0C) = 0;
    B(D_800F0F80, 0x0E) = (unsigned char)dith;
    B(D_800F0F80, 0x0F) = 0;
    B(D_800F0F80, 0x10) = 0;
    func_800AEB98(D_800F0F80 - 8);

    H(D_800F0FD8, 0x00) = 0;
    H(D_800F0FD8, 0x02) = 0;
    H(D_800F0FD8, 0x04) = (unsigned short)w;
    H(D_800F0FD8, 0x06) = (unsigned short)h;
    H(D_800F0FD8, 0x08) = 0;
    H(D_800F0FD8, 0x0A) = 0;
    H(D_800F0FD8, 0x0C) = 0;
    H(D_800F0FD8, 0x0E) = 0;

    if (func_800BB22C() == 1) {
        H(D_800F0FD8, 0x0A) = 0x18;
        B(D_800F0FD8, 0x12) = 1;
    }

    B(D_800F0FD8, 0x10) = (unsigned char)(flags & 1u);
    D_8010CD80 = (signed short)(flags & 4u);
    B(D_800F0FD8, 0x11) = (unsigned char)rgb24;
    func_800AED64(D_800F0FD8);
}
