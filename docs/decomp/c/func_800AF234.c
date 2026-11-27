/*
 * func_800AF234
 *
 * Purpose:
 *   Builds a PS1 GPU DR_AREA primitive packet (equivalent to PsyQ SetDrawArea)
 *   from a RECT struct. Sets the primitive packet word count byte at +0x03 to 2,
 *   encodes the GP0(0xE3) top-left draw area command at +0x04 via
 *   func_800AF820(rect->x, rect->y), and encodes the GP0(0xE4) bottom-right
 *   draw area command at +0x08 via func_800AF8B8(rect->x + rect->w - 1,
 *   rect->y + rect->h - 1).
 *
 * Arguments:
 *   dr_area (unsigned char *)       - destination DR_AREA primitive buffer
 *   rect    (const unsigned char *) - source RECT (s16 x at +0, s16 y at +2, u16 w at +4, u16 h at +6)
 *
 * Return value:
 *   None (void)
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(const unsigned short *)((p) + (o)))
#define SH(p, o) (*(const signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))

extern unsigned int func_800AF820(signed short x, signed short y);
extern unsigned int func_800AF8B8(signed short x, signed short y);

void func_800AF234(unsigned char *dr_area, const unsigned char *rect)
{
    signed short x1;
    signed short y1;

    B(dr_area, 0x03) = 2;
    W(dr_area, 0x04) = func_800AF820(SH(rect, 0x00), SH(rect, 0x02));

    x1 = (signed short)(H(rect, 0x00) + H(rect, 0x04) - 1);
    y1 = (signed short)(H(rect, 0x02) + H(rect, 0x06) - 1);
    W(dr_area, 0x08) = func_800AF8B8(x1, y1);
}
