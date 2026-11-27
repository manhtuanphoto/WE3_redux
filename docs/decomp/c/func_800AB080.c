/*
 * func_800AB080
 *
 * Purpose:
 *   Recomputes the 39-entry (0..38) channel left/right volume pairs in
 *   `D_800CE86C` (8-byte stride, +0x00 and +0x01) by scaling each base volume
 *   `D_800F1040[i]` by the master factor `D_800CE738[D_800E6AA0]`, truncating
 *   the product to signed 16-bit, and dividing by 128 (with negative rounding
 *   toward zero: `+ 0x7F` before `>> 7`).
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Global fields read/written:
 *   D_800E6AA0 (s16)       master volume index
 *   D_800CE738 (s16[])     master volume scale table
 *   D_800F1040 (u8[39])    per-channel base volume table
 *   D_800CE86C (u8[39][8]) channel parameter table (+0x00 left, +0x01 right written)
 */

extern signed short D_800E6AA0;
extern signed short D_800CE738[];
extern unsigned char D_800F1040[];
extern unsigned char D_800CE86C[];

void func_800AB080(void)
{
    int i;

    for (i = 0; i < 0x27; i++) {
        signed short prod = (signed short)(D_800F1040[i] * D_800CE738[D_800E6AA0]);
        unsigned int scaled;

        if (prod < 0) {
            scaled = ((unsigned int)(int)(prod + 0x7F)) >> 7;
        } else {
            scaled = ((unsigned int)(int)prod) >> 7;
        }

        D_800CE86C[(i << 3) + 0] = (unsigned char)scaled;
        D_800CE86C[(i << 3) + 1] = (unsigned char)scaled;
    }
}
