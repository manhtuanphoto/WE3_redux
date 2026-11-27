/*
 * func_800B6378
 *
 * Purpose:
 *   Copies a 6-element signed 32-bit vector pair (24 bytes, e.g. camera eye
 *   and target positions) from `src` to `dst`, arithmetic-right-shifting all 6
 *   components by (bits - 15) if the bit-width returned by
 *   func_800B652C(func_800B6464(src)) is >= 16, to avoid 32-bit overflow in
 *   subsequent distance/GTE calculations.
 *
 * Arguments:
 *   src (u8*) - pointer to 6 x s32 input coordinates (24 bytes)
 *   dst (u8*) - pointer to 6 x s32 output coordinates (24 bytes)
 *
 * Return value:
 *   None (void)
 */

#define SW(p, o) (*(signed int *)((p) + (o)))

extern int func_800B6464(unsigned char *src);
extern int func_800B652C(int val);

void func_800B6378(unsigned char *src, unsigned char *dst)
{
    int bitLen;
    int shift;

    bitLen = func_800B652C(func_800B6464(src));
    if (bitLen < 16) {
        SW(dst, 0x00) = SW(src, 0x00);
        SW(dst, 0x04) = SW(src, 0x04);
        SW(dst, 0x08) = SW(src, 0x08);
        SW(dst, 0x0C) = SW(src, 0x0C);
        SW(dst, 0x10) = SW(src, 0x10);
        SW(dst, 0x14) = SW(src, 0x14);
    } else {
        shift = bitLen - 15;
        SW(dst, 0x00) = SW(src, 0x00) >> shift;
        SW(dst, 0x04) = SW(src, 0x04) >> shift;
        SW(dst, 0x08) = SW(src, 0x08) >> shift;
        SW(dst, 0x0C) = SW(src, 0x0C) >> shift;
        SW(dst, 0x10) = SW(src, 0x10) >> shift;
        SW(dst, 0x14) = SW(src, 0x14) >> shift;
    }
}
