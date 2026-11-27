/*
 * func_800ADF68
 *
 * Purpose:
 *   Builds a PS1 GPU GP0(0xA0) Image Load (CPU-to-VRAM rectangle copy) DMA
 *   packet from a RECT structure. Computes 32-bit pixel word count
 *   (w * h + 1) / 2; if word count < 13 (0xD), sets packet DMA tag length byte
 *   (+0x03) to (words + 4), otherwise sets it to 0. Writes command 0xA0000000
 *   at +0x04, (y << 16 | x) at +0x08, (h << 16 | w) at +0x0C, writes
 *   0x01000000 at pkt + (len * 4), and returns the pointer pkt + (len * 4).
 *
 * Arguments:
 *   pkt  (u8 *):        destination GPU DMA packet buffer
 *   rect (const s16 *): RECT {s16 x, y, w, h}
 *
 * Return value:
 *   unsigned char *: pointer to pkt + (len * 4) where 0x01000000 was written
 */

#define B(p, o) (*(unsigned char *)((p) + (o)))
#define S(p, o) (*(const signed short *)((p) + (o)))
#define W(p, o) (*(unsigned int *)((p) + (o)))

unsigned char *func_800ADF68(unsigned char *pkt, const unsigned char *rect)
{
    signed int pixels = (signed int)S(rect, 0x04) * (signed int)S(rect, 0x06);
    signed int words = (pixels + 1) / 2;
    unsigned int len;
    unsigned char *end;

    if ((unsigned int)words < 0xDu) {
        len = (unsigned int)(words + 4);
    } else {
        len = 0u;
    }

    B(pkt, 0x03) = (unsigned char)len;
    W(pkt, 0x04) = 0xA0000000u; /* GP0(0xA0): Copy Rectangle (CPU -> VRAM) */
    W(pkt, 0x08) = W(rect, 0x00);
    W(pkt, 0x0C) = W(rect, 0x04);

    end = pkt + (len << 2);
    W(end, 0x00) = 0x01000000u;
    return end;
}
