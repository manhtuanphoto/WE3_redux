/*
 * func_800C1C88 (plus two unlabeled adjacent leaf functions in the same .s unit)
 *
 * Purpose:
 *   SPU / memory-block address/size alignment and hardware flag helpers:
 *   1) func_800C1C88: Aligns size/offset `a1` up to D_800D55B8 when D_800D55B0 != 0
 *      using mask ~D_800D55BC, shifts right by D_800D55B4 to compute block index/size.
 *      If slot == -2, returns aligned `a1`; if slot == -1, returns shifted value & 0xFFFF;
 *      otherwise stores shifted value into D_800D558C[slot] and returns aligned `a1`.
 *   2) func_800C1D2C (unlabeled at +0xA4): Reads D_800D558C[slot]; if a1 == -1 returns
 *      the raw u16 value, else returns it shifted left by D_800D55B4.
 *   3) func_800C1D68 (unlabeled at +0xE0): Updates bits 16..18 of *D_800D559C to
 *      0x30000 (if a0 != 0) or 0x50000 (if a0 == 0).
 *
 * Global fields read/written:
 *   D_800D55B0 (unsigned int): alignment enable flag
 *   D_800D55B4 (unsigned int): bit shift (e.g., 3 for 8-byte SPU units)
 *   D_800D55B8 (unsigned int): alignment modulus (divisor; traps on 0)
 *   D_800D55BC (unsigned int): alignment mask (modulus - 1)
 *   D_800D558C (unsigned short *): block/offset table
 *   D_800D559C (volatile unsigned int *): hardware/control register pointer
 *
 * Notes:
 *   UNCLEAR: Exact symbol names for the two unlabeled adjacent functions at +0xA4 and +0xE0.
 */

extern unsigned int D_800D55B0;
extern unsigned int D_800D55B4;
extern unsigned int D_800D55B8;
extern unsigned int D_800D55BC;
extern unsigned short *D_800D558C;
extern volatile unsigned int *D_800D559C;

unsigned int func_800C1C88(int slot, unsigned int val)
{
    unsigned int shifted;

    if (D_800D55B0 != 0) {
        unsigned int align = D_800D55B8;
        /* Assembly executes divu + break 7 if align == 0 */
        if ((val % align) != 0) {
            val = (val + align) & ~D_800D55BC;
        }
    }

    shifted = val >> D_800D55B4;

    if (slot == -2) {
        return val;
    }
    if (slot == -1) {
        return shifted & 0xFFFFu;
    }

    D_800D558C[slot] = (unsigned short)shifted;
    return val;
}

/* UNCLEAR: unlabeled adjacent function at +0xA4 in func_800C1C88.s */
unsigned int func_800C1D2C(int slot, int raw_flag)
{
    unsigned int val = D_800D558C[slot];

    if (raw_flag == -1) {
        return val;
    }
    return val << D_800D55B4;
}

/* UNCLEAR: unlabeled adjacent function at +0xE0 in func_800C1C88.s */
void func_800C1D68(int mode)
{
    unsigned int reg = *D_800D559C & 0xFFF8FFFFu;
    *D_800D559C = reg;

    if (mode != 0) {
        *D_800D559C = *D_800D559C | 0x00030000u;
    } else {
        *D_800D559C = *D_800D559C | 0x00050000u;
    }
}
