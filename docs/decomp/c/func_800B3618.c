/*
 * func_800B3618 - Emit a PS1 GPU Gouraud-shaded line primitive (LINE_G2)
 *
 * Purpose:
 *   Reads a line descriptor struct at `desc`, builds a 5-word PS1 GPU LINE_G2
 *   packet (command 0x50, optionally semi-transparent bit 1 from flags bit 30,
 *   plus DR_MODE/texpage word 0xE1000200 with ABR bits from flags[29:28]),
 *   applies global screen offsets (D_800F1070, D_800F1072) to both endpoints,
 *   links it into the ordering table via func_800B3578, and advances D_800EF8A8.
 *
 * Arguments:
 *   desc (unsigned char *): line descriptor struct:
 *     +0x00 s32 flags (bit 31 = hidden/skip; bit 30 -> cmd bit 1 semi-trans;
 *                      bits 29:28 -> DR_MODE bits 6:5 ABR)
 *     +0x04 u16 x0, +0x06 u16 y0
 *     +0x08 u16 x1, +0x0A u16 y1
 *     +0x0C u8  r0, +0x0D u8 g0, +0x0E u8 b0
 *     +0x0F u8  r1, +0x10 u8 g1, +0x11 u8 b1
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

void func_800B3618(unsigned char *desc, void *ot_ptr, unsigned int ot_slot)
{
    signed int flags = SW(desc, 0x00);
    unsigned char *pkt;

    if (flags < 0) {
        return;
    }

    pkt = D_800EF8A8;

    /* PS1 GPU DR_MODE (0xE1000200 = dither on) with ABR semi-transparency bits [6:5] */
    W(pkt, 0x04) = 0xE1000200u | ((unsigned int)(flags >> 23) & 0x60u);

    /* Vertex 0 color + LINE_G2 command (0x50 | semi-transparent bit 1) */
    B(pkt, 0x08) = B(desc, 0x0C);
    B(pkt, 0x09) = B(desc, 0x0D);
    B(pkt, 0x0A) = B(desc, 0x0E);
    B(pkt, 0x0B) = (unsigned char)(((unsigned int)(flags >> 29) & 2u) | 0x50u);

    /* Vertex 0 screen position */
    H(pkt, 0x0C) = (unsigned short)(H(desc, 0x04) + D_800F1070);
    H(pkt, 0x0E) = (unsigned short)(H(desc, 0x06) + D_800F1072);

    /* Vertex 1 color */
    B(pkt, 0x10) = B(desc, 0x0F);
    B(pkt, 0x11) = B(desc, 0x10);
    B(pkt, 0x12) = B(desc, 0x11);

    /* Vertex 1 screen position */
    H(pkt, 0x14) = (unsigned short)(H(desc, 0x08) + D_800F1070);
    H(pkt, 0x16) = (unsigned short)(H(desc, 0x0A) + D_800F1072);

    D_800EF8A8 = func_800B3578(pkt, ot_ptr, ot_slot & 0xFFFFu, 5);
}
