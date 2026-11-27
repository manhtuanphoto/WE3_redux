/*
 * func_800C2E28 (plus 1 trailing unlabeled memory-region check helper in func_800C2E28.s)
 *
 * Purpose:
 *   1. func_800C2E28: Enables or disables an SPU control bit (0x80 at *(D_800D558C + 0x1AA))
 *      and updates D_800D5120. When mode == 0, clears the bit and sets D_800D5120 = 0.
 *      When mode == 1, sets the bit and D_800D5120 = 1 unless D_800D5124 != 1 and
 *      func_800C2F78(D_800D5128) == 0, in which case it clears it.
 *   2. func_800C2EFC (unlabeled at +0xD4): Walks 8-byte descriptor entries in
 *      D_800D55F4 (skipping entries with bit 31 set, stopping at sentinel bit 30)
 *      and checks whether `addr` falls below `entryAddr + entrySize` for any active
 *      entry (returning 1 if found/below, 0 otherwise).
 *
 * Global fields read / written:
 *   D_800D558C (u8 *): SPU control/attribute structure (+0x1AA u16 flags)
 *   D_800D5120 (s32): current enabled state (0 or 1)
 *   D_800D5124 (s32), D_800D5128 (u32): state/resource check parameters
 *   D_800D55F4 (u32 *): 8-byte-entry memory/SPU allocation table
 */

extern unsigned char *D_800D558C;
extern signed int D_800D5120;
extern signed int D_800D5124;
extern unsigned int D_800D5128;
extern unsigned int *D_800D55F4;

extern signed int func_800C2F78(unsigned int arg0);

signed int func_800C2E28(signed int mode)
{
    unsigned char *ctrl;
    unsigned short flags;

    if (mode == 0) {
        ctrl = D_800D558C;
        flags = *(unsigned short *)(ctrl + 0x1AA);
        D_800D5120 = 0;
        *(unsigned short *)(ctrl + 0x1AA) = (unsigned short)(flags & 0xFF7FU);
    } else if (mode == 1) {
        if (D_800D5124 != mode && func_800C2F78(D_800D5128) == 0) {
            ctrl = D_800D558C;
            flags = *(unsigned short *)(ctrl + 0x1AA);
            D_800D5120 = 0;
            *(unsigned short *)(ctrl + 0x1AA) = (unsigned short)(flags & 0xFF7FU);
        } else {
            ctrl = D_800D558C;
            flags = *(unsigned short *)(ctrl + 0x1AA);
            D_800D5120 = mode;
            *(unsigned short *)(ctrl + 0x1AA) = (unsigned short)(flags | 0x0080U);
        }
    }

    return D_800D5120;
}

/* Trailing unlabeled function in func_800C2E28.s (0x800C2EFC) */
signed int func_800C2EFC(unsigned int addr)
{
    unsigned int *entry = D_800D55F4;

    if (entry == 0) {
        return 0;
    }

    for (;;) {
        unsigned int header = entry[0];
        if ((header & 0x80000000U) == 0) {
            unsigned int baseAddr;
            if ((header & 0x40000000U) != 0) {
                return 0;
            }
            baseAddr = header & 0x0FFFFFFFU;
            if (addr <= baseAddr || addr < (baseAddr + entry[1])) {
                return 1;
            }
        }
        entry += 2;
    }
}
