/*
 * func_80071AF8 - Initialize UI / HUD sprites and quad primitive vertex bounds
 *
 * Purpose:
 *   Clears D_800E6D52, registers six UI/HUD sprite/primitive entries (slots 1..6)
 *   via func_8001CDAC, and directly sets signed 16-bit screen/vertex coordinates
 *   on primitives returned by func_8001D004 for IDs 0x66, 0x67, and 0x68
 *   (e.g., gauge/bar width scaled by arg0).
 *
 * Arguments:
 *   arg0        (u8)  - bar/scale factor (used as arg0 << 12 and arg0 << 4)
 *   unused_arg1 (u32) - unused ($a1 is overwritten before being read)
 *   arg2        (u8)  - primitive/texture ID for slot 1
 *   arg3        (u8)  - primitive/texture ID for slot 2
 *   arg4        (u8)  - stack argument (0x10($caller_sp)): base x/y/flag passed to slots 3..6
 *
 * Return value:
 *   None (void).
 *
 * Globals written:
 *   D_800E6D52 (u8) - cleared to 0
 */

#define S(p, o) (*(signed short *)((p) + (o)))

extern unsigned char D_800E6D52;

extern void func_8001CDAC(int slot, int id, int flags, int a3,
                          int s0, int s1, int s2, int s3,
                          int s4, int s5, int s6, int r,
                          int g, int b, int s10, int s11);
extern unsigned char *func_8001D004(int id);

void func_80071AF8(unsigned char arg0, unsigned int unused_arg1,
                   unsigned char arg2, unsigned int arg3, unsigned char arg4)
{
    unsigned int scale12;
    int x0, x1;
    signed short y0, y1;
    unsigned char *prim;

    (void)unused_arg1;

    D_800E6D52 = 0;

    func_8001CDAC(1, arg2 & 0xFF, 0, 0,
                  0, 8, 0, 0x1000, 0, 0, 0, 0x80, 0x80, 0x80, 0, 1);

    func_8001CDAC(2, arg3 & 0xFF, 0, 0,
                  0, 8, 0, 0x1000, 0, 0, 0, 0x80, 0x80, 0x80, 0, 1);

    arg0 &= 0xFF;
    scale12 = (unsigned int)arg0 << 12;
    arg4 &= 0xFF;

    func_8001CDAC(3, 0x69, 0x40000000, 0,
                  arg4, 8, 0, (int)scale12, 0, 0, 0, 0x80, 0x80, 0x80, 0, 1);

    func_8001CDAC(4, 0x66, 0x40000000, 0,
                  arg4, 8, 0, (int)scale12, 0, 0, 0, 0x80, 0x80, 0x80, 0, 1);

    prim = func_8001D004(0x66);
    y0 = -0x10;
    y1 = 8;
    S(prim, 0x08) = y0;
    S(prim, 0x14) = y1;

    func_8001CDAC(5, 0x67, 0x40000000, 0,
                  arg4, 8, 0, 0x1000, 0, 0, 0, 0x80, 0x80, 0x80, 0, 1);

    prim = func_8001D004(0x67);
    x1 = (int)(arg0 << 4);
    x0 = -8 - x1;
    S(prim, 0x06) = (signed short)x0;
    S(prim, 0x12) = (signed short)x1;
    S(prim, 0x08) = -8;
    S(prim, 0x14) = -8;

    func_8001CDAC(6, 0x68, 0x40000000, 0,
                  arg4, 8, 0, 0x1000, 0, 0, 0, 0x80, 0x80, 0x80, 0, 1);

    prim = func_8001D004(0x68);
    S(prim, 0x06) = (signed short)x0;
    S(prim, 0x1E) = (signed short)x0;
    S(prim, 0x12) = (signed short)x1;
    S(prim, 0x2A) = (signed short)x1;
    S(prim, 0x08) = y0;
    S(prim, 0x14) = y0;
    S(prim, 0x20) = y1;
    S(prim, 0x2C) = y1;
}
