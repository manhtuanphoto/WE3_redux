/*
 * func_800C1028
 *
 * Purpose:
 *   Transfers a chunk of audio/VAB data (analogous to PsyQ SsVabTransBodyPartly)
 *   for VAB slot `slot` (0..16). If no transfer is active (D_800D50FC == 0),
 *   initiates SPU transfer to address D_8010CDE0[slot] with total size
 *   D_8010CD98[slot]. Transfers up to `size` bytes (clamped to remaining
 *   D_800D50FC) from `src`, updating remaining byte count and marking
 *   D_8010A3F0[slot] = 1 upon completion.
 *
 * Arguments:
 *   src  (void *): source buffer pointer
 *   size (unsigned int): maximum bytes to transfer in this call
 *   slot (signed short): VAB slot index (0..16)
 *
 * Return value:
 *   slot (>= 0) when the transfer finishes;
 *   -2 while bytes remain to be transferred;
 *   -1 if slot >= 17, D_8010A3F0[slot] != 2, or another slot is active.
 *
 * Global fields read / written:
 *   D_8010A3F0 (u8[17]): VAB slot state (2 = ready for body transfer, 1 = complete)
 *   D_800D50FC (u32): remaining bytes in active VAB transfer
 *   D_800D5100 (s16): active VAB slot index (-1 when idle)
 *   D_8010CD98 (u32[]): total body size per VAB slot
 *   D_8010CDE0 (u32[]): SPU target address per VAB slot
 */

extern unsigned char D_8010A3F0[];
extern unsigned int D_800D50FC;
extern signed short D_800D5100;
extern unsigned int D_8010CD98[];
extern unsigned int D_8010CDE0[];

extern void func_800C3E18(signed int arg0);
extern void func_800C3DB8(unsigned int spuAddr);
extern void func_800C3F88(signed int mode);
extern void func_800C3E48(void *src, unsigned int size);

signed int func_800C1028(void *src, unsigned int size, signed short slot)
{
    signed int slotIdx;
    signed int activeSlot;
    unsigned int chunkSize;

    if ((unsigned short)slot >= 0x11U) {
        func_800C3F88(0);
        return -1;
    }

    slotIdx = (signed int)slot;
    if (D_8010A3F0[slotIdx] != 2) {
        func_800C3F88(0);
        return -1;
    }

    if (D_800D50FC == 0) {
        D_800D5100 = slot;
        D_800D50FC = D_8010CD98[slotIdx];
        func_800C3E18(0);
        func_800C3DB8(D_8010CDE0[slotIdx]);
    }

    activeSlot = (signed int)D_800D5100;
    if (activeSlot != slotIdx) {
        func_800C3F88(0);
        return -1;
    }

    chunkSize = size;
    if (D_800D50FC < chunkSize) {
        chunkSize = D_800D50FC;
    }

    func_800C3F88(1);
    func_800C3E48(src, chunkSize);
    D_800D50FC -= chunkSize;

    if (D_800D50FC != 0) {
        return -2;
    }

    D_800D5100 = -1;
    D_800D50FC = 0;
    D_8010A3F0[activeSlot] = 1;
    return activeSlot;
}
