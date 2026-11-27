/*
 * func_800C2F78
 *
 * Purpose:
 *   Shifts input value left by D_800D55B4 bits and searches an 8-byte-entry
 *   table pointed to by D_800D55F4. Entries with bit 31 set (0x80000000) are
 *   skipped, and an entry with bit 30 set (0x40000000) terminates the search
 *   returning 0. For active entries, masks bits [27:0] as base and compares
 *   shifted value against [base, base + size).
 *
 * Arguments:
 *   val (unsigned int) - input value/index shifted left by D_800D55B4
 *
 * Return value:
 *   1 if an active entry satisfies (val <= base || val < base + size);
 *   0 if D_800D55F4 is NULL or the 0x40000000 terminator is reached.
 *
 * Global fields read:
 *   D_800D55B4 (u32)   - left-shift count applied to val
 *   D_800D55F4 (u32 *) - pointer to array of 8-byte { flags_and_base, size } entries
 */

struct D_800D55F4_Entry {
    unsigned int flags_and_base;
    unsigned int size;
};

extern unsigned int D_800D55B4;
extern struct D_800D55F4_Entry *D_800D55F4;

int func_800C2F78(unsigned int val)
{
    struct D_800D55F4_Entry *entry = D_800D55F4;
    unsigned int shifted = val << D_800D55B4;

    if (entry == 0) {
        return 0;
    }

    for (;;) {
        unsigned int word0 = entry->flags_and_base;

        if ((word0 & 0x80000000U) == 0) {
            unsigned int base;

            if ((word0 & 0x40000000U) != 0) {
                return 0;
            }

            base = word0 & 0x0FFFFFFFU;
            if (shifted <= base || shifted < base + entry->size) {
                return 1;
            }
        }

        entry++;
    }
}
