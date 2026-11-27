/*
 * func_800C2838
 *
 * Purpose:
 *   SPU SRAM fixed-address allocator (equivalent to libspu `SpuMallocWithStartAddr`).
 *   Aligns the requested `addr` and `size` to the SPU SRAM block granularity
 *   (D_800D55B4 shift / D_800D55BC mask), checks bounds against the reserved
 *   SPU reverb workspace (when D_800D5124 != 0) and minimum SPU RAM base 0x1010,
 *   and either splits the initial unallocated block in D_800D55F4 (if bit 30 is
 *   set) or searches the block table for a matching free region and inserts the
 *   allocation via func_800C24D8.
 *
 * Arguments:
 *   addr (int) - requested SPU SRAM byte address (>= 0x1010 after alignment)
 *   size (int) - requested byte size
 *
 * Return value:
 *   int - aligned allocated SPU SRAM address on success, or -1 on failure.
 *
 * Global fields read/written:
 *   D_800D5124 (u32)  - reverb active flag
 *   D_800D5128 (u32)  - reverb work-area base (in SPU words/units)
 *   D_800D55B4 (u32)  - alignment shift (3 -> 8-byte units)
 *   D_800D55BC (u32)  - alignment mask (7)
 *   D_800D55EC (s32)  - max allocation table entries
 *   D_800D55F0 (s32)  - current allocation table entry count
 *   D_800D55F4 (u32*) - allocation block table (pairs of {addr_flags, size})
 *     bit 31 (0x80000000): free/gap marker
 *     bit 30 (0x40000000): end-of-table / trailing free block marker
 */

extern int D_800D5124;
extern unsigned int D_800D5128;
extern unsigned int D_800D55B4;
extern unsigned int D_800D55BC;
extern int D_800D55EC;
extern int D_800D55F0;
extern unsigned int *D_800D55F4;

extern void func_800C21D8(int arg);
extern int func_800C24D8(int index, int block_addr, int size);

int func_800C2838(int addr, int size)
{
    int reverb_reserve;
    unsigned int mask;
    unsigned int not_mask;
    int shift;
    int aligned_addr;
    int aligned_size;
    int end_addr;
    unsigned int *table;

    if (D_800D5124 != 0) {
        reverb_reserve = (int)((0x10000U - D_800D5128) << D_800D55B4);
    } else {
        reverb_reserve = 0;
    }

    mask = D_800D55BC;
    not_mask = ~mask;
    aligned_size = size;
    if ((aligned_size & not_mask) != 0) {
        aligned_size += (int)mask;
    }

    shift = (int)D_800D55B4;
    aligned_addr = addr;
    aligned_size = (aligned_size >> shift) << shift;
    if ((aligned_addr & not_mask) != 0) {
        aligned_addr += (int)mask;
    }
    aligned_addr = (aligned_addr >> shift) << shift;

    if (aligned_addr < 0x1010) {
        return -1;
    }

    end_addr = aligned_addr + aligned_size;
    if (((int)(0x10000U << shift) - reverb_reserve) < end_addr) {
        return -1;
    }

    table = D_800D55F4;
    if ((table[0] & 0x40000000U) != 0) {
        int pre_gap = aligned_addr - 0x1010;
        int post_size = ((int)table[1] + 0x1010 - aligned_addr) - aligned_size;

        if (pre_gap > 0) {
            if (D_800D55EC - 2 < D_800D55F0) {
                return -1;
            }
            table[0] = 0x80001010U;
            table[4] = (unsigned int)end_addr | 0x40000000U;
            table[1] = (unsigned int)pre_gap;
            table[2] = (unsigned int)aligned_addr;
            table[3] = (unsigned int)aligned_size;
            table[5] = (unsigned int)post_size;
            D_800D55F0 = 2;
        } else {
            if (D_800D55EC - 1 < D_800D55F0) {
                return -1;
            }
            table[2] = (unsigned int)end_addr | 0x40000000U;
            table[0] = (unsigned int)aligned_addr;
            table[1] = (unsigned int)aligned_size;
            table[3] = (unsigned int)post_size;
            D_800D55F0 = 1;
        }

        /* Note: assembly places `v0 = aligned_addr` in delay slot of jal func_800C21D8 */
        func_800C21D8(0);
        return aligned_addr;
    } else {
        int idx = 0;
        unsigned int block_addr;

        func_800C21D8(0);
        for (;;) {
            unsigned int *entry = D_800D55F4 + idx * 2;
            unsigned int raw = entry[0];
            unsigned int is_end = raw & 0x40000000U;

            if ((int)raw < 0) {
                block_addr = raw & 0x0FFFFFFFU;
                if ((unsigned int)aligned_addr >= block_addr) {
                    if ((unsigned int)aligned_addr >= block_addr + entry[1] ||
                        (entry[2] & 0x40000000U) != 0) {
                        unsigned int *e = D_800D55F4 + idx * 2;
                        int avail = (int)e[1] - (aligned_addr - (int)(e[0] & 0x0FFFFFFFU));
                        if (avail >= aligned_size) {
                            block_addr = (unsigned int)aligned_addr;
                            break;
                        }
                    }
                } else {
                    if ((int)entry[1] >= aligned_size) {
                        break;
                    }
                }
            } else if (is_end != 0) {
                int avail = (int)entry[1] - reverb_reserve;
                if (avail < aligned_size) {
                    return -1;
                }
                block_addr = raw & 0x0FFFFFFFU;
                break;
            }

            idx++;
        }

        aligned_addr = func_800C24D8(idx, (int)block_addr, aligned_size);
        /* Note: assembly places `v0 = aligned_addr` in delay slot of jal func_800C21D8 */
        func_800C21D8(0);
        return aligned_addr;
    }
}
