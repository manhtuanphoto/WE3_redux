/*
 * func_8006F528
 *
 * Purpose:
 *   Cycles byte field +0x15 in the 40-byte entry D_80105508[slot_idx] to the next
 *   element in `options[0 .. count - 1]`. First checks whether any of the lowest
 *   min(bit_limit, 7) + 1 bits in Scratchpad+0x290 are set; if so, returns 0 without
 *   modifying the entry. Otherwise finds the current value of +0x15 in `options`,
 *   advances to `(i + 1) % count` (or `options[0]` if not found or `count == 0`),
 *   writes it to +0x15, and returns 1.
 *
 * Arguments:
 *   slot_idx (unsigned char): index into 40-byte table D_80105508
 *   options (const unsigned char *): array of candidate byte values
 *   count (unsigned char): number of elements in `options`
 *   bit_limit (unsigned char): bit range guard in Scratchpad+0x290 (clamped to 7)
 *
 * Return value:
 *   1 if +0x15 was updated, 0 if blocked by Scratchpad+0x290 bitmask.
 *
 * Global fields read/written:
 *   D_80105508 (40-byte stride struct array; +0x15 u8)
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x290 (u32): bitmask guard
 */

#define B(p, o) (*(unsigned char *)((p) + (o)))
#define W(p, o) (*(unsigned int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_80105508[];

unsigned int func_8006F528(
    unsigned char slot_idx,
    const unsigned char *options,
    unsigned char count,
    unsigned char bit_limit)
{
    unsigned char *entry = D_80105508 + ((unsigned int)(slot_idx & 0xFF) * 40U);
    unsigned int lim = bit_limit & 0xFF;
    signed int shift;
    unsigned int mask;
    unsigned int cnt;
    unsigned int i;
    unsigned int next_idx;

    if (lim >= 8U) {
        lim = 7U;
    }
    shift = 7 - (signed int)lim;
    mask = (unsigned int)(0xFF >> shift);

    if ((W(Scratchpad, 0x290) & mask) != 0) {
        return 0;
    }

    cnt = count & 0xFF;
    next_idx = 0;

    if (cnt > 0) {
        unsigned char cur = B(entry, 0x15);
        i = 0;
        do {
            if (cur == options[i & 0xFF]) {
                break;
            }
            i++;
        } while ((i & 0xFF) < cnt);

        i &= 0xFF;
        if (i < cnt) {
            next_idx = (i + 1U) % cnt;
        }
    }

    B(entry, 0x15) = options[next_idx & 0xFF];
    return 1;
}
