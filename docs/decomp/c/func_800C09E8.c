/*
 * func_800C09E8
 *
 * Purpose:
 *   Scans tone entries in the sound bank table pointed to by D_800F20B0 for
 *   program D_8010A3C8[7] across indices 0 .. D_8010A3C8[0] - 1. For each tone
 *   whose [min, max] range (+0x06..+0x07) contains key/note D_8010A3C8[2],
 *   appends the tone index to `outIndices` and tone[+0x16] to `outAttrs`.
 *
 * Arguments:
 *   outIndices (u8*) - output array receiving matched tone indices
 *   outAttrs   (u8*) - output array receiving tone+0x16 bytes
 *
 * Return value:
 *   u32 - number of matched entries (masked to 8 bits)
 *
 * Globals read:
 *   D_8010A3C8 (u8[], R) - sound query struct (+0x00 s8 count, +0x02 s8 key, +0x07 s8 program)
 *   D_800F20B0 (u8*, R)   - pointer to 32-byte tone attribute table (16 tones per program)
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define SB(p, o) (*(signed char *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))

extern unsigned char D_8010A3C8[];
extern unsigned char D_800F20B0[];

unsigned int func_800C09E8(unsigned char *outIndices, unsigned char *outAttrs)
{
    unsigned char *query = D_8010A3C8;
    int matchCount = 0;
    signed char count = SB(query, 0);

    if (count > 0) {
        int idx = 0;
        do {
            signed char prog = SB(query, 7);
            signed char key = SB(query, 2);
            unsigned char *tone = (unsigned char *)W(D_800F20B0, 0) + (((prog << 4) + (signed char)idx) << 5);

            if ((int)key >= (int)B(tone, 6) && (int)key <= (int)B(tone, 7)) {
                unsigned int outPos = (unsigned int)matchCount & 0xFFU;
                outAttrs[outPos] = B(tone, 0x16);
                outIndices[outPos] = (unsigned char)idx;
                matchCount++;
            }
            idx++;
        } while ((signed char)idx < SB(query, 0));
    }

    return (unsigned int)matchCount & 0xFFU;
}
