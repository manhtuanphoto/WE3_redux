/*
 * func_800BC3D8 - Reset/stop a PsyQ SEQ/MIDI track entry in D_8010C2A8
 *
 * Purpose:
 *   Indexes the 176-byte (0xB0) sequence track struct at
 *   D_8010C2A8[seq_idx] + track_idx * 0xB0, sets loop limit +0x20 = 1 and loop
 *   counter +0x21 = 0, clears status flag bits (0x100, 0x8, 0x2, 0x4, 0x200)
 *   in +0x98, resets current stream pointer +0x00 to start pointer +0x04,
 *   sets +0x14 = 1, sets bit 0x1 in +0x98, and returns the updated +0x98 flags.
 *
 * Arguments:
 *   seq_idx (signed short):   sequence slot index into D_8010C2A8
 *   track_idx (signed short): track index (0xB0-byte stride)
 *
 * Return value:
 *   unsigned int: updated track flags word (+0x98)
 *
 * Globals read:
 *   D_8010C2A8 (u8 *[]): array of 0xB0-stride sequence track struct tables
 */

#define B(p, o) (*(unsigned char *)((p) + (o)))
#define W(p, o) (*(unsigned int *)((p) + (o)))

extern unsigned char *D_8010C2A8[];

unsigned int func_800BC3D8(signed short seq_idx, signed short track_idx)
{
    unsigned char *track = D_8010C2A8[seq_idx] + ((signed int)track_idx * 0xB0);
    unsigned int flags;

    B(track, 0x20) = 1;
    B(track, 0x21) = 0;

    W(track, 0x98) &= ~0x100u;
    W(track, 0x98) &= ~0x8u;
    W(track, 0x98) &= ~0x2u;
    W(track, 0x98) &= ~0x4u;
    W(track, 0x98) &= ~0x200u;

    W(track, 0x00) = W(track, 0x04);
    B(track, 0x14) = 1;

    flags = W(track, 0x98) | 0x1u;
    W(track, 0x98) = flags;
    return flags;
}
