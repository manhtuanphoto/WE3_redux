/*
 * func_800AF96C
 *
 * Purpose:
 *   Encodes a PS1 GPU GP0(0xE2) Texture Window Setting command word from a
 *   RECT struct (equivalent to PsyQ GetTexWindow / SetTexWindow helper).
 *   Returns 0 if `rect` is NULL. Otherwise extracts 8-pixel-unit 5-bit fields:
 *     - offset X (bits 10..14) = ((u8)rect->x) >> 3
 *     - offset Y (bits 15..19) = ((u8)rect->y) >> 3
 *     - mask X   (bits  0.. 4) = ((-rect->w) & 0xFF) >> 3
 *     - mask Y   (bits  5.. 9) = ((-rect->h) & 0xFF) >> 3
 *   and ORs them with 0xE2000000.
 *
 * Arguments:
 *   rect (const unsigned char *) - pointer to RECT (+0x00 s16 x, +0x02 s16 y, +0x04 s16 w, +0x06 s16 h), or NULL
 *
 * Return value:
 *   unsigned int - encoded GP0(0xE2) command word, or 0 if rect == NULL
 */

#define B(p, o)  (*(const unsigned char *)((p) + (o)))
#define SH(p, o) (*(const signed short *)((p) + (o)))

unsigned int func_800AF96C(const unsigned char *rect)
{
    unsigned int off_x;
    unsigned int off_y;
    unsigned int mask_x;
    unsigned int mask_y;

    if (rect == 0) {
        return 0;
    }

    off_x = (unsigned int)B(rect, 0x00) >> 3;
    mask_x = (unsigned int)((-SH(rect, 0x04)) & 0xFF) >> 3;
    off_y = (unsigned int)B(rect, 0x02) >> 3;
    mask_y = (unsigned int)((-SH(rect, 0x06)) & 0xFF) >> 3;

    return 0xE2000000u | (off_y << 15) | (off_x << 10) | (mask_y << 5) | mask_x;
}
