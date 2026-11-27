/*
 * func_800BF1FC
 *
 * Purpose:
 *   Scans all active/allocated sound/SEQ voice slots `0..D_800FB578-1` (skipping
 *   slots marked free in bitmask D_800D4F0C) in the 54-byte-stride table starting
 *   at D_800E78D8. For every slot matching (a0, a1, a2, a3) at fields (+0x10,
 *   +0x18, +0x14, +0x0E relative to D_800E78D8), stops or updates it via
 *   func_800BFCD8 (if field +0x00 == 0xFF) or sets D_8010A3E2 = slot and calls
 *   func_800BFD18(0).
 *
 * Arguments:
 *   a0 (signed short): matched against D_800E78E8[slot * 54]
 *   a1 (signed short): matched against D_800E78F0[slot * 54]
 *   a2 (signed short): matched against D_800E78EC[slot * 54]
 *   a3 (unsigned short): zero-extended and matched against D_800E78E6[slot * 54]
 *
 * Return value:
 *   Number of matching voice slots processed.
 *
 * Global fields read / written:
 *   D_800FB578 (s8): total slot count
 *   D_800D4F0C (u32): bitmask of inactive/free slots (bit i == 1 skips slot i)
 *   D_800E78D8, D_800E78E6, D_800E78E8, D_800E78EC, D_800E78F0 (s16[], stride 54 bytes)
 *   D_8010A3E2 (s16): selected slot index for func_800BFD18
 */

extern signed char D_800FB578;
extern unsigned int D_800D4F0C;
extern unsigned char D_800E78D8[];
extern unsigned char D_800E78E6[];
extern unsigned char D_800E78E8[];
extern unsigned char D_800E78EC[];
extern unsigned char D_800E78F0[];
extern signed short D_8010A3E2;

extern void func_800BFCD8(void);
extern void func_800BFD18(signed int arg0);

signed int func_800BF1FC(signed short a0, signed short a1, signed short a2, unsigned short a3)
{
    unsigned int slot = 0;
    signed int count = 0;
    signed int target3 = (signed int)(unsigned short)a3;
    signed int target2 = (signed int)a2;
    signed int target0 = (signed int)a0;
    signed int target1 = (signed int)a1;

    if (D_800FB578 <= 0) {
        return 0;
    }

    while ((signed int)(slot & 0xFFU) < (signed int)D_800FB578) {
        unsigned int idx = slot & 0xFFU;
        if ((D_800D4F0C & (1U << idx)) == 0) {
            unsigned int off = idx * 54U;
            if (*(signed short *)(D_800E78E6 + off) == target3 &&
                *(signed short *)(D_800E78EC + off) == target2 &&
                *(signed short *)(D_800E78E8 + off) == target0 &&
                *(signed short *)(D_800E78F0 + off) == target1) {
                if (*(signed short *)(D_800E78D8 + off) == 0xFF) {
                    func_800BFCD8();
                } else {
                    D_8010A3E2 = (signed short)idx;
                    func_800BFD18(0);
                }
                count++;
            }
        }
        slot++;
    }

    return count;
}
