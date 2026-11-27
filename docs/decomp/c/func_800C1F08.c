/*
 * func_800C1F08
 *
 * Purpose:
 *   Allocates a block of size reqSize (aligned to 1 << D_800D55B4) from the
 *   memory/buffer descriptor table D_800D55F4 (8 bytes per entry: [0]=flags|addr,
 *   [1]=size), splitting a free/tail block if needed and compacting via func_800C21D8.
 *
 * Arguments:
 *   reqSize: (unsigned int) requested allocation size in bytes/units
 *
 * Return value:
 *   (unsigned int) allocated block address/descriptor word, or -1 (0xFFFFFFFF) on failure
 *
 * Reads / Writes:
 *   D_800D5124, D_800D5128 (u32): tail reserve parameters (R)
 *   D_800D55B4 (u32): alignment shift (R)
 *   D_800D55BC (u32): alignment mask/bias (R)
 *   D_800D55EC (s32): maximum number of entries in D_800D55F4 (R)
 *   D_800D55F0 (s32): next/tail cursor index in D_800D55F4 (R/W)
 *   D_800D55F4 (u32*): descriptor table of 2-word entries (R/W)
 *
 * Magic numbers:
 *   0x40000000: top/tail free-pool block flag.
 *   0x80000000: split free-fragment block flag.
 *   0x0FFFFFFF: 28-bit address/offset mask.
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned int D_800D5124;
extern unsigned int D_800D5128;
extern unsigned int D_800D55B4;
extern unsigned int D_800D55BC;
extern signed int D_800D55EC;
extern signed int D_800D55F0;
extern unsigned int *D_800D55F4;
extern void func_800C21D8(void);

unsigned int func_800C1F08(unsigned int reqSize) {
    unsigned int reserved;
    signed int chosen;
    signed int i;
    unsigned int *entry;

    if (D_800D5124 == 0u) {
        reserved = 0u;
    } else {
        reserved = (0x10000u - D_800D5128) << D_800D55B4;
    }

    if ((reqSize & ~D_800D55BC) != 0u) {
        reqSize += D_800D55BC;
    }
    reqSize = (unsigned int)((signed int)reqSize >> D_800D55B4) << D_800D55B4;

    chosen = -1;
    if ((D_800D55F4[0] & 0x40000000u) != 0u) {
        chosen = 0;
    } else {
        func_800C21D8();
        for (i = 0; i < D_800D55EC; i++) {
            unsigned int flags = D_800D55F4[i * 2];
            if ((flags & 0x40000000u) != 0u ||
                ((flags & 0x80000000u) != 0u && D_800D55F4[i * 2 + 1] >= reqSize)) {
                chosen = i;
                break;
            }
        }
    }

    if (chosen == -1) {
        return 0xFFFFFFFFu;
    }

    entry = &D_800D55F4[chosen * 2];
    if ((entry[0] & 0x40000000u) != 0u) {
        signed int nextIdx;
        unsigned int *nextEntry;

        if (chosen >= D_800D55EC) {
            return 0xFFFFFFFFu;
        }
        if (entry[1] - reserved < reqSize) {
            return 0xFFFFFFFFu;
        }

        nextIdx = chosen + 1;
        nextEntry = &D_800D55F4[nextIdx * 2];
        nextEntry[0] = ((entry[0] & 0x0FFFFFFFu) + reqSize) | 0x40000000u;
        nextEntry[1] = entry[1] - reqSize;

        D_800D55F0 = nextIdx;
        entry[1] = reqSize;
        entry[0] &= 0x0FFFFFFFu;
        func_800C21D8();
        return D_800D55F4[chosen * 2];
    }

    {
        unsigned int entryFlags = entry[0];
        unsigned int entrySize = entry[1];
        if (reqSize < entrySize && D_800D55F0 < D_800D55EC) {
            signed int tailIdx = D_800D55F0;
            unsigned int *tail = &D_800D55F4[tailIdx * 2];
            unsigned int prev0 = tail[0];
            unsigned int prev1 = tail[1];
            tail[0] = (entryFlags + reqSize) | 0x80000000u;
            tail[1] = entrySize - reqSize;
            D_800D55F0 = tailIdx + 1;
            tail[2] = prev0;
            tail[3] = prev1;
        }
    }

    entry = &D_800D55F4[chosen * 2];
    entry[1] = reqSize;
    entry[0] &= 0x0FFFFFFFu;
    func_800C21D8();
    return D_800D55F4[chosen * 2];
}
