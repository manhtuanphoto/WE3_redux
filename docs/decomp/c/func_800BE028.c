/*
 * func_800BE028
 *
 * Purpose:
 *   Allocates or selects a slot (0..D_800FB578-1) from a 54-byte-per-entry
 *   cache/resource table rooted at D_800E78DA: picks the first completely empty
 *   unlocked slot, or evicts the lowest-priority / oldest LRU slot, ages all
 *   unlocked slots, and initializes the chosen slot with D_8010A3D7.
 *
 * Arguments:
 *   void
 *
 * Return value:
 *   (unsigned int) chosen slot index (0..255), or 0x63 (99) if D_800FB578 <= 0
 *
 * Reads / Writes:
 *   D_800FB578 (s8): total number of slots (R)
 *   D_800D4F0C (u32): bitmask of locked/pinned slots (R)
 *   D_8010A3D7 (s8): priority/tag byte stored into selected slot's +0x18 (R)
 *   D_800E78DA (54-byte stride table, 0x36 bytes/entry):
 *     +0x00 (D_800E78DA, u16/s16): LRU age counter (R/W)
 *     +0x04 (D_800E78DE, u16): secondary tie-breaker key (R)
 *     +0x18 (D_800E78F2, s16/u16): priority / tag (R/W)
 *     +0x1B (D_800E78F5, s8): active/occupied flag (R)
 *     +0x1C (D_800E78F6, s16): cleared to 0 on selected slot (W)
 *     +0x28 (D_800E7902, s16): cleared to 0 on selected slot (W)
 *
 * Magic numbers:
 *   0x36 (54): byte stride per table entry.
 *   0x63 (99): sentinel "no free slot found" index.
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_8010A3D7;
extern signed char D_800FB578;
extern unsigned int D_800D4F0C;
extern unsigned char D_800E78DA[];

unsigned int func_800BE028(void) {
    unsigned int chosen = 0x63;
    unsigned short bestKey = 0xFFFF;
    unsigned int tieCount = 0;
    unsigned short bestAge = 0;
    unsigned int bestIdx = 0x63;
    signed int bestPrio;
    signed int slotCount;
    unsigned int i;

    bestPrio = (signed char)D_8010A3D7;
    slotCount = D_800FB578;

    if (slotCount > 0) {
        unsigned int lockedMask = D_800D4F0C;
        for (i = 0; (signed int)(i & 0xFFu) < slotCount; i++) {
            unsigned int idx = i & 0xFFu;
            unsigned int off;
            signed short prio;

            if ((lockedMask & (1u << idx)) != 0u) {
                continue;
            }

            off = idx * 54u;
            if ((signed char)B(D_800E78DA, off + 0x1Bu) == 0 && H(D_800E78DA, off + 0x04u) == 0u) {
                chosen = i;
                break;
            }

            prio = S(D_800E78DA, off + 0x18u);
            if (prio < (signed int)(bestPrio & 0xFFFFu)) {
                bestPrio = H(D_800E78DA, off + 0x18u);
                bestIdx = i;
                bestKey = H(D_800E78DA, off + 0x04u);
                bestAge = H(D_800E78DA, off + 0x00u);
                tieCount = 1;
            } else if (prio == (signed int)(bestPrio & 0xFFFFu)) {
                unsigned short key = H(D_800E78DA, off + 0x04u);
                if (key < (bestKey & 0xFFFFu)) {
                    tieCount++;
                    bestAge = H(D_800E78DA, off + 0x00u);
                    bestKey = key;
                    bestIdx = i;
                } else if (key == (bestKey & 0xFFFFu)) {
                    tieCount++;
                    if ((signed short)bestAge < S(D_800E78DA, off + 0x00u)) {
                        bestAge = H(D_800E78DA, off + 0x00u);
                        bestIdx = i;
                    }
                }
            }
        }
    }

    if ((chosen & 0xFFu) == 0x63u && (tieCount & 0xFFu) == 0u) {
        chosen = (unsigned char)D_800FB578;
    } else if ((chosen & 0xFFu) == 0x63u) {
        chosen = bestIdx;
    }

    slotCount = D_800FB578;
    if ((signed int)(chosen & 0xFFu) < slotCount) {
        if (slotCount > 0) {
            unsigned int lockedMask = D_800D4F0C;
            for (i = 0; (signed int)(i & 0xFFu) < slotCount; i++) {
                unsigned int idx = i & 0xFFu;
                if ((lockedMask & (1u << idx)) == 0u) {
                    unsigned int off = idx * 54u;
                    H(D_800E78DA, off + 0x00u) = (unsigned short)(H(D_800E78DA, off + 0x00u) + 1u);
                }
            }
        }
        {
            unsigned int off = (chosen & 0xFFu) * 54u;
            H(D_800E78DA, off + 0x00u) = 0;
            H(D_800E78DA, off + 0x28u) = 0;
            H(D_800E78DA, off + 0x1Cu) = 0;
            H(D_800E78DA, off + 0x18u) = (unsigned short)(signed short)(signed char)D_8010A3D7;
        }
    }

    return chosen & 0xFFu;
}
