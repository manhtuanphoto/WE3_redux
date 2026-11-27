/*
 * func_800C24D8
 *
 * Purpose:
 *   PsyQ-style SPU heap block allocator (_SpuMalloc2-like). Allocates a region
 *   of size `size` at `addr` inside block `blockIdx` of the allocator table
 *   D_800D55F4 (8-byte entries: { u32 addr_and_flags; s32 size; }). Splits the
 *   block into prefix/allocated/suffix entries as needed, updates D_800D55F0
 *   (active entry count), calls func_800C21D8 (table coalesce/sort), and
 *   returns `addr` on success or -1 on failure.
 *
 * Arguments:
 *   blockIdx: (int) index into D_800D55F4
 *   addr:     (int) requested start address within block
 *   size:     (int) requested allocation size
 *
 * Return value:
 *   (int) allocated `addr` on success, or -1 if insufficient space / table full
 *
 * Reads / Writes:
 *   Globals:
 *     D_800D55F4 (u32* pointer to block table, stride 8 bytes) (R/W)
 *     D_800D55F0 (s32 current block count) (R/W)
 *     D_800D55EC (s32 max block capacity) (R)
 *     D_800D5124 (s32 alignment/guard flag) (R)
 *     D_800D5128 (s32) (R)
 *     D_800D55B4 (s32 shift) (R)
 *
 * Magic numbers:
 *   0x0FFFFFFF = address mask (low 28 bits of entry word 0)
 *   0x40000000 = tail/sentinel block flag in entry word 0
 *   0x80000000 = free-block flag in entry word 0
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char* D_800D55F4;
extern int D_800D55F0;
extern int D_800D55EC;
extern int D_800D5124;
extern int D_800D5128;
extern int D_800D55B4;

extern void func_800C21D8(void);

int func_800C24D8(int blockIdx, int addr, int size) {
    unsigned char* table = D_800D55F4;
    unsigned char* entry = table + blockIdx * 8;
    unsigned int word0 = W(entry, 0);
    int blockSize = SW(entry, 4);
    int baseAddr = (int)(word0 & 0x0FFFFFFFU);
    int offset = addr - baseAddr;
    int guard;
    int avail;

    if (D_800D5124 != 0) {
        guard = (0x10000 - D_800D5128) << D_800D55B4;
    } else {
        guard = 0;
    }

    if (word0 & 0x40000000U) {
        avail = (blockSize - offset) - guard;
    } else {
        avail = blockSize - offset;
    }

    if (avail < size) {
        return -1;
    }

    if (word0 & 0x40000000U) {
        int count = D_800D55F0;
        if (offset > 0) {
            unsigned char* cur;
            unsigned char* next;
            if (D_800D55EC - 2 < count) {
                return -1;
            }
            table = D_800D55F4;
            D_800D55F0 = count + 1;
            cur = table + count * 8;
            next = table + (count + 1) * 8;
            SW(cur, 4) = offset;
            W(cur, 0) = (W(cur, 0) & 0x0FFFFFFFU) | 0x80000000U;
            SW(next, 0) = addr;
            SW(next, 4) = size;
        } else {
            unsigned char* cur;
            if (D_800D55EC - 1 < count) {
                return -1;
            }
            table = D_800D55F4;
            cur = table + count * 8;
            SW(cur, 4) = size;
            W(cur, 0) &= 0x0FFFFFFFU;
        }
        {
            int idx = D_800D55F0 + 1;
            unsigned char* tail;
            table = D_800D55F4;
            D_800D55F0 = idx;
            tail = table + idx * 8;
            W(tail, 0) = ((word0 & 0x0FFFFFFFU) + (unsigned int)offset + (unsigned int)size) | 0x40000000U;
            SW(tail, 4) = (blockSize - offset) - size;
        }
    } else {
        if (offset > 0) {
            int limit = (avail == size) ? (D_800D55EC - 2) : (D_800D55EC - 1);
            int count = D_800D55F0;
            unsigned char* cur;
            unsigned int old0;
            int old1;

            if (limit < count) {
                return -1;
            }
            table = D_800D55F4;
            SW(table + blockIdx * 8, 4) = offset;
            cur = table + count * 8;
            old0 = W(cur, 0);
            old1 = SW(cur, 4);
            SW(cur, 0) = addr;
            SW(cur, 4) = size;
            D_800D55F0 = count + 1;
            W(cur, 8) = old0;
            SW(cur, 0xC) = old1;

            if (size <= avail) {
                unsigned char* next = table + (count + 1) * 8;
                unsigned int nOld0 = W(next, 0);
                int nOld1 = SW(next, 4);
                W(next, 0) = ((word0 & 0x0FFFFFFFU) + (unsigned int)offset + (unsigned int)size) | 0x80000000U;
                SW(next, 4) = avail - size;
                D_800D55F0 = count + 2;
                W(next, 8) = nOld0;
                SW(next, 0xC) = nOld1;
            }
        } else {
            if (size < avail && D_800D55EC - 1 < D_800D55F0) {
                return -1;
            }
            table = D_800D55F4;
            entry = table + blockIdx * 8;
            SW(entry, 4) = size;
            W(entry, 0) &= 0x0FFFFFFFU;
            if (size < avail) {
                int count = D_800D55F0;
                unsigned char* next = table + count * 8;
                unsigned int old0 = W(next, 0);
                int old1 = SW(next, 4);
                W(next, 0) = ((word0 & 0x0FFFFFFFU) + (unsigned int)size) | 0x80000000U;
                SW(next, 4) = avail - size;
                D_800D55F0 = count + 1;
                W(next, 8) = old0;
                SW(next, 0xC) = old1;
            }
        }
    }

    func_800C21D8();
    return addr;
}
