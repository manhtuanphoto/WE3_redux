/*
 * func_800BBB68
 *
 * Purpose:
 *   Stops or resets a sequencer track in D_8010C2A8[seq_idx][track_idx]
 *   (0xB0-byte stride per track). Calls func_800C0930((s16)(seq_idx | (track_idx << 8))),
 *   clears the track status byte at +0x14 to 0, and clears bit 1 (mask ~0x2)
 *   in the track flags word at +0x98.
 *
 * Arguments:
 *   seq_idx   (signed short): sequence/bank index into D_8010C2A8
 *   track_idx (signed short): track index (0xB0-byte stride)
 *
 * Return value:
 *   None (void)
 *
 * Globals read:
 *   D_8010C2A8 (unsigned char *[]): array of pointers to track tables (0xB0-byte stride)
 */

#define W(p, o) (*(unsigned int *)((p) + (o)))

extern unsigned char *D_8010C2A8[];

extern void func_800C0930(signed short packed_id);

void func_800BBB68(signed short seq_idx, signed short track_idx)
{
    signed int track_offset = (signed int)track_idx * 0xB0;
    unsigned char *track = D_8010C2A8[seq_idx] + track_offset;
    signed short packed_id = (signed short)((unsigned int)(unsigned short)seq_idx | ((unsigned int)(unsigned short)track_idx << 8));

    func_800C0930(packed_id);

    track[0x14] = 0;

    track = D_8010C2A8[seq_idx] + track_offset;
    W(track, 0x98) &= ~0x2U;
}
