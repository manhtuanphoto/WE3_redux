/*
 * func_800B3728 - Emit a PS1 GPU monochrome rectangle primitive (TILE)
 *
 * Purpose:
 *   Reads a rectangle descriptor struct at `desc`, builds a 4-word PS1 GPU TILE
 *   packet (command 0x60, optionally semi-transparent bit 1 from flags bit 30,
 *   plus DR_MODE/texpage word 0xE1000200 with flags[25:24] -> bits[8:7] and
 *   flags[29:28] -> bits[6:5]), adds global screen offsets (D_800F1070,
 *   D_800F1072) to (x, y), copies (w, h), links it into the ordering table via
 *   func_800B3578, and advances D_800EF8A8.
 *
 * Arguments:
 *   desc (unsigned char *): rectangle descriptor struct:
 *     +0x00 s32 flags (bit 31 = hidden/skip; bit 30 -> cmd bit 1 semi-trans;
 *                      bits 29:28 -> DR_MODE bits 6:5; bits 25:24 -> DR_MODE bits 8:7)
 *     +0x04 u16 x, +0x06 u16 y
 *     +0x08 u16 w, +0x0A u16 h
 *     +0x0C u8  r, +0x0D u8  g, +0x0E u8 b
 *   ot_ptr (void *):        ordering table entry pointer passed to func_800B3578
 *   ot_slot (unsigned int): 16-bit OT depth/slot index
 *
 * Return value:
 *   None (void)
 *
 * Globals read & written:
 *   D_800EF8A8 (unsigned char *, R/W): current GPU packet buffer pointer
 *   D_800F1070 (u16/s16, R): global screen X offset
 *   D_800F1072 (u16/s16, R): global screen Y offset
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))

extern unsigned char *D_800EF8A8;
extern unsigned short D_800F1070;
extern unsigned short D_800F1072;

extern unsigned char *func_800B3578(unsigned char *pkt, void *ot_ptr, unsigned int ot_slot, int word_count);

void func_800B3728(unsigned char *desc, void *ot_ptr, unsigned int ot_slot)
{
    signed int flags = SW(desc, 0x00);
    unsigned char *pkt;
    unsigned int drmode;

    if (flags < 0) {
        return;
    }

    pkt = D_800EF8A8;

    /* PS1 GPU DR_MODE (0xE1000200 = dither on) with texpage/ABR bits [8:5] */
    drmode = 0xE1000200u
           | ((unsigned int)(flags >> 17) & 0x180u)
           | ((unsigned int)(flags >> 23) & 0x60u);
    W(pkt, 0x04) = drmode;

    /* Color + TILE command (0x60 | semi-transparent bit 1) */
    B(pkt, 0x08) = B(desc, 0x0C);
    B(pkt, 0x09) = B(desc, 0x0D);
    B(pkt, 0x0A) = B(desc, 0x0E);
    B(pkt, 0x0B) = (unsigned char)(((unsigned int)(flags >> 29) & 2u) | 0x60u);

    H(pkt, 0x0C) = (unsigned short)(H(desc, 0x04) + D_800F1070);
    H(pkt, 0x0E) = (unsigned short)(H(desc, 0x06) + D_800F1072);
    H(pkt, 0x10) = H(desc, 0x08);
    H(pkt, 0x12) = H(desc, 0x0A);

    D_800EF8A8 = func_800B3578(pkt, ot_ptr, ot_slot & 0xFFFFu, 4);
}
