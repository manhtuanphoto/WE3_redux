/*
 * func_800B5248
 *
 * Purpose:
 *   Configures the two framebuffer display buffer origin coordinates in
 *   `D_800E7800` (X0, X1) and `D_800E789C` (Y0, Y1). Depending on `D_8010CD80`,
 *   either clears `D_800E81F8` / `D_800E8200` to 0 (`D_8010CD80 != 0`) or
 *   mirrors `(x0, x1)` and `(y0, y1)` into them (`D_8010CD80 == 0`), then
 *   calls `func_800B47C8()` and `func_800B4698()`.
 *
 * Arguments:
 *   x0 (signed short) - buffer 0 display X
 *   y0 (signed short) - buffer 0 display Y
 *   x1 (signed short) - buffer 1 display X
 *   y1 (signed short) - buffer 1 display Y
 *
 * Return value:
 *   None (void)
 *
 * Global fields read/written:
 *   D_8010CD80 (s16)    flag selecting whether secondary offsets are zeroed or mirrored
 *   D_800E7800 (s16[2]) display buffer X coordinates (+0x00 x0, +0x02 x1)
 *   D_800E789C (s16[2]) display buffer Y coordinates (+0x00 y0, +0x02 y1)
 *   D_800E81F8 (s16[2]) secondary X coordinates (+0x00, +0x02)
 *   D_800E8200 (s16[2]) secondary Y coordinates (+0x00, +0x02)
 */

#define SH(p, o) (*(signed short *)((p) + (o)))

extern signed short D_8010CD80;
extern unsigned char D_800E7800[];
extern unsigned char D_800E789C[];
extern unsigned char D_800E81F8[];
extern unsigned char D_800E8200[];

extern void func_800B47C8(void);
extern void func_800B4698(void);

void func_800B5248(signed short x0, signed short y0, signed short x1, signed short y1)
{
    signed short zeroSecondary = D_8010CD80;

    SH(D_800E7800, 0x00) = x0;
    SH(D_800E7800, 0x02) = x1;
    SH(D_800E789C, 0x00) = y0;
    SH(D_800E789C, 0x02) = y1;

    if (zeroSecondary != 0) {
        SH(D_800E81F8, 0x00) = 0;
        SH(D_800E81F8, 0x02) = 0;
        SH(D_800E8200, 0x00) = 0;
        SH(D_800E8200, 0x02) = 0;
    } else {
        SH(D_800E81F8, 0x00) = x0;
        SH(D_800E81F8, 0x02) = x1;
        SH(D_800E8200, 0x00) = y0;
        SH(D_800E8200, 0x02) = y1;
    }

    func_800B47C8();
    func_800B4698();
}
