/*
 * func_800C083C - Read SEQ/SEP track fields +0x58 and +0x5A (+ two adjacent getters)
 *
 * Purpose:
 *   `func_800C083C` unpacks a 16-bit sequence/track ID (`low byte` = slot into
 *   D_8010C2A8, `high byte` = track index with 0xB0-byte stride), saves the
 *   packed ID into D_8010A3DE, writes the track's +0x58 and +0x5A halfwords to
 *   `out_a` and `out_b`, and returns (s16)D_8010A3DE.
 *
 *   Two unlabeled adjacent functions follow immediately in the `.s` file:
 *     - Second entry (lines 24-40): stores D_8010A3DE and returns (s16)+0x58.
 *     - Third entry  (lines 41-57): stores D_8010A3DE and returns (s16)+0x5A.
 *
 * Arguments:
 *   packed_id (unsigned short): low byte = seq slot, high byte = track index
 *   out_a (unsigned short *):   receives track +0x58
 *   out_b (unsigned short *):   receives track +0x5A
 *
 * Return value:
 *   (signed short) D_8010A3DE for func_800C083C; +0x58 / +0x5A for the getters.
 *
 * Globals read & written:
 *   D_8010C2A8 (u8 *[], R): array of 0xB0-stride sequence track struct tables
 *   D_8010A3DE (s16, R/W): last queried packed sequence/track ID
 */

#define H(p, o) (*(unsigned short *)((p) + (o)))
#define S(p, o) (*(signed short *)((p) + (o)))

extern unsigned char *D_8010C2A8[];
extern signed short D_8010A3DE;

signed short func_800C083C(unsigned short packed_id, unsigned short *out_a, unsigned short *out_b)
{
    unsigned int slot = (unsigned int)(packed_id & 0xFFu);
    signed int track = (signed int)((packed_id & 0xFF00u) >> 8);
    unsigned char *entry;

    D_8010A3DE = (signed short)packed_id;
    entry = D_8010C2A8[slot] + (track * 0xB0);

    *out_a = H(entry, 0x58);
    *out_b = H(entry, 0x5A);

    return D_8010A3DE;
}

/* UNCLEAR: unlabeled second function in func_800C083C.s (lines 24-40) */
signed short func_800C089C(unsigned short packed_id)
{
    unsigned int slot = (unsigned int)(packed_id & 0xFFu);
    signed int track = (signed int)((packed_id & 0xFF00u) >> 8);
    unsigned char *entry;

    D_8010A3DE = (signed short)packed_id;
    entry = D_8010C2A8[slot] + (track * 0xB0);

    return S(entry, 0x58);
}

/* UNCLEAR: unlabeled third function in func_800C083C.s (lines 41-57) */
signed short func_800C08E0(unsigned short packed_id)
{
    unsigned int slot = (unsigned int)(packed_id & 0xFFu);
    signed int track = (signed int)((packed_id & 0xFF00u) >> 8);
    unsigned char *entry;

    D_8010A3DE = (signed short)packed_id;
    entry = D_8010C2A8[slot] + (track * 0xB0);

    return S(entry, 0x5A);
}
