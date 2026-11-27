/*
 * func_800C1EA8
 *
 * Purpose:
 *   PsyQ SPU memory allocator initialization (SpuInitMalloc-style). If count
 *   <= 0, returns 0 immediately. Otherwise initializes the free-block header at
 *   table[0] = 0x40001010 and table[1] = (0x10000 << D_800D55B4) - 0x1010, and
 *   records the allocator table base, active count (0), and max block count.
 *
 * Arguments:
 *   count (signed int):   maximum number of SPU malloc entries (must be > 0)
 *   table (unsigned int *): pointer to SPU malloc block table buffer
 *
 * Return value:
 *   signed int: count if > 0, or 0 if count <= 0.
 *
 * Global fields read/written:
 *   D_800D55B4 (u32): SPU RAM size shift (0x10000 << D_800D55B4 total bytes)
 *   D_800D55EC (s32): max SPU malloc entries (set to count)
 *   D_800D55F0 (u32): current allocated block count (cleared to 0)
 *   D_800D55F4 (u32*): SPU malloc table base pointer (set to table)
 */

extern unsigned int D_800D55B4;
extern signed int D_800D55EC;
extern unsigned int D_800D55F0;
extern unsigned int *D_800D55F4;

signed int func_800C1EA8(signed int count, unsigned int *table)
{
    unsigned int shift;

    if (count <= 0) {
        return 0;
    }

    shift = D_800D55B4;
    table[0] = 0x40001010u;
    D_800D55F4 = table;
    D_800D55F0 = 0;
    D_800D55EC = count;
    table[1] = (0x10000u << shift) - 0x1010u;

    return count;
}
