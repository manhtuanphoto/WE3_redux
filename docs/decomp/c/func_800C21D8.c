/*
 * func_800C21D8
 *
 * Purpose:
 *   Coalesces, sorts, compacts, and trims memory-block / heap descriptors in
 *   the 8-byte-entry array D_800D55F4 (active last index D_800D55F0).
 *   1) Merges adjacent free blocks (bit 0x80000000 set) whose base+size
 *      matches the next non-tombstone free block, marking absorbed entries
 *      with tombstone 0x2FFFFFFF.
 *   2) Marks zero-size entries with 0x2FFFFFFF.
 *   3) Sorts non-end entries (bit 0x40000000 clear) in ascending order of low
 *      28-bit address.
 *   4) Truncates D_800D55F0 at the first tombstone by moving the last entry into
 *      that slot.
 *   5) Folds trailing free blocks before D_800D55F0 back into the preceding
 *      block with flag 0x40000000 and decrements D_800D55F0.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   void
 *
 * Global fields read/written:
 *   D_800D55F0 (s32) last valid index in block table (-1 if empty),
 *   D_800D55F4 (u32 *) pointer to array of 8-byte { u32 tag_addr, u32 size }
 *
 * Scratchpad fields read/written:
 *   None
 *
 * Magic numbers:
 *   0x80000000 - free block flag
 *   0x40000000 - end/tail block flag
 *   0x2FFFFFFF - deleted/tombstone block tag
 *   0x0FFFFFFF - 28-bit block address mask
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern signed int D_800D55F0;
extern unsigned int *D_800D55F4;

void func_800C21D8(void)
{
    int i;
    int j;
    int last;
    unsigned int *table;

    /* Pass 1: merge contiguous free blocks (0x80000000). */
    last = D_800D55F0;
    if (last >= 0) {
        table = D_800D55F4;
        i = 0;
        do {
            unsigned int *cur = &table[i * 2];
            if ((cur[0] & 0x80000000U) != 0) {
                unsigned int *next;
                j = i + 1;
                while (table[j * 2] == 0x2FFFFFFFU) {
                    j++;
                }
                next = &table[j * 2];
                if ((next[0] & 0x80000000U) != 0) {
                    if ((next[0] & 0x0FFFFFFFU) == ((cur[0] & 0x0FFFFFFFU) + cur[1])) {
                        next[0] = 0x2FFFFFFFU;
                        cur[1] += next[1];
                        continue;
                    }
                }
            }
            i++;
        } while (i <= last);
    }

    /* Pass 2: mark zero-length entries as tombstones (0x2FFFFFFF). */
    last = D_800D55F0;
    if (last >= 0) {
        table = D_800D55F4;
        for (i = 0; i <= last; i++) {
            if (table[i * 2 + 1] == 0) {
                table[i * 2] = 0x2FFFFFFFU;
            }
        }
    }

    /* Pass 3: sort entries before the 0x40000000 sentinel by address. */
    if (D_800D55F0 >= 0) {
        table = D_800D55F4;
        for (i = 0; i <= D_800D55F0; i++) {
            if ((table[i * 2] & 0x40000000U) != 0) {
                break;
            }
            for (j = i + 1; j <= D_800D55F0; j++) {
                if ((table[j * 2] & 0x40000000U) != 0) {
                    break;
                }
                if ((table[j * 2] & 0x0FFFFFFFU) < (table[i * 2] & 0x0FFFFFFFU)) {
                    unsigned int tmp_tag = table[i * 2];
                    unsigned int tmp_sz = table[i * 2 + 1];
                    table[i * 2] = table[j * 2];
                    table[i * 2 + 1] = table[j * 2 + 1];
                    table[j * 2] = tmp_tag;
                    table[j * 2 + 1] = tmp_sz;
                }
            }
        }
    }

    /* Pass 4: replace first tombstone before 0x40000000 with last entry and trim. */
    if (D_800D55F0 >= 0) {
        table = D_800D55F4;
        for (i = 0; i <= D_800D55F0; i++) {
            if ((table[i * 2] & 0x40000000U) != 0) {
                break;
            }
            if (table[i * 2] == 0x2FFFFFFFU) {
                int tail = D_800D55F0;
                table[i * 2] = table[tail * 2];
                table[i * 2 + 1] = table[tail * 2 + 1];
                D_800D55F0 = i;
                break;
            }
        }
    }

    /* Pass 5: fold trailing free blocks into tail sentinel and shrink count. */
    table = D_800D55F4;
    for (i = D_800D55F0 - 1; i >= 0; i--) {
        unsigned int tag = table[i * 2];
        int old_last;
        if ((tag & 0x80000000U) == 0) {
            break;
        }
        table[i * 2] = (tag & 0x0FFFFFFFU) | 0x40000000U;
        old_last = D_800D55F0;
        D_800D55F0 = i;
        table[i * 2 + 1] += table[old_last * 2 + 1];
    }
}
