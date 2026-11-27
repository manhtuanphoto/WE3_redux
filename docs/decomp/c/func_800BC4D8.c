/*
 * func_800BC4D8
 *
 * Purpose:
 *   Marks a sequencer track for replay/restart in the 0xB0-byte-stride track
 *   table at D_8010C2A8[seq]: sets byte +0x14 to 1 and clears bit 3 (0x08) in
 *   the track flags word at +0x98.
 *
 * Arguments:
 *   seq   (signed short): sequence/bank index into D_8010C2A8
 *   track (signed short): track index within the sequence table (0xB0 stride)
 *
 * Return value:
 *   None (void)
 *
 * Global fields read/written:
 *   D_8010C2A8 (unsigned char *[]): array of pointers to 0xB0-byte track structs
 *     +0x14 (u8)  replay/pending trigger flag set to 1
 *     +0x98 (u32) track status bitmask (bit 3 / 0x08 cleared)
 */

#define B(p, o) (*(unsigned char *)((p) + (o)))
#define W(p, o) (*(unsigned int *)((p) + (o)))

extern unsigned char *D_8010C2A8[];

void func_800BC4D8(signed short seq, signed short track)
{
    signed int trackOff = (signed int)track * 0xB0;
    unsigned char *entry;

    entry = D_8010C2A8[seq] + trackOff;
    B(entry, 0x14) = 1;

    entry = D_8010C2A8[seq] + trackOff;
    W(entry, 0x98) &= ~0x08U;
}
