/*
 * func_800B4138
 *
 * Purpose:
 *   Full graphics subsystem initialization (PsyQ GsInitGraph-style wrapper).
 *   Masks (w, h, flags, dith, rgb24) to 16 bits, configures GPU/DRAWENV/DISPENV
 *   via func_800B41AC, calls func_800B5B78, resets active buffer index
 *   D_8010CD54 = 0, initializes 3D projection/aspect matrices via
 *   func_800B4330(w, h), and invokes func_800B47C8 and func_800B4698.
 *
 * Arguments:
 *   w     (unsigned short): display width
 *   h     (unsigned short): display height
 *   flags (unsigned short): graph mode flags
 *   dith  (unsigned short): dither enable flag
 *   rgb24 (unsigned short): 24-bit color mode flag (passed on stack at +0x10)
 *
 * Return value:
 *   None (void)
 *
 * Global fields written:
 *   D_8010CD54 (signed short): active display/draw buffer index = 0
 */

extern signed short D_8010CD54;

extern void func_800B41AC(unsigned int w, unsigned int h, unsigned int flags,
                          unsigned int dith, unsigned short rgb24);
extern void func_800B5B78(void);
extern void func_800B4330(unsigned short width, unsigned short height);
extern void func_800B47C8(void);
extern void func_800B4698(void);

void func_800B4138(
    unsigned short w,
    unsigned short h,
    unsigned short flags,
    unsigned short dith,
    unsigned short rgb24)
{
    unsigned int width = w & 0xFFFFU;
    unsigned int height = h & 0xFFFFU;

    func_800B41AC(width, height, flags & 0xFFFFU, dith & 0xFFFFU, rgb24);
    func_800B5B78();
    D_8010CD54 = 0;
    func_800B4330((unsigned short)width, (unsigned short)height);
    func_800B47C8();
    func_800B4698();
}
