/*
 * func_800BC328
 *
 * Purpose:
 *   Reads a MIDI/SEQ variable-length quantity (VLQ) delta-time from the track's
 *   current stream pointer (*(u8 **)track) in D_8010C2A8[seq_idx][track_idx]
 *   (0xB0-byte stride per track), advancing the pointer past the VLQ bytes. If
 *   the first byte is non-zero, decodes the 7-bit-per-byte VLQ and adds
 *   (val * 10) to the track's 32-bit field at +0x88.
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

void func_800BC328(signed short seq_idx, signed short track_idx)
{
    unsigned char *track = D_8010C2A8[seq_idx] + ((signed int)track_idx * 0xB0);
    unsigned char *stream = (unsigned char *)W(track, 0x00);
    unsigned int byte = (unsigned int)(*stream++);
    unsigned int val;

    W(track, 0x00) = (unsigned int)stream;
    if (byte == 0) {
        return;
    }

    if (byte & 0x80U) {
        unsigned int next;
        val = byte & 0x7FU;
        do {
            stream = (unsigned char *)W(track, 0x00);
            next = (unsigned int)(*stream++);
            W(track, 0x00) = (unsigned int)stream;
            val = (val << 7) + (next & 0x7FU);
        } while (next & 0x80U);
    } else {
        val = byte;
    }

    W(track, 0x88) += val * 10U;
}
