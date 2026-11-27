/*
 * func_800C2DD8
 *
 * Purpose:
 *   Clamps `clock` to [0, 0x3F] (0..63) and writes it into bits 8..13 of the
 *   SPU control register at D_800D558C + 0x1AA (SPUCNT 0x1F801DAA noise clock
 *   field; PsyQ SpuSetNoiseClock equivalent). Returns the clamped value.
 *
 * Arguments:
 *   clock (signed int): requested 6-bit noise clock step (0..63)
 *
 * Return value:
 *   signed int: clamped value written to bits 8..13 of SPUCNT (0..63)
 *
 * Global fields read/written:
 *   D_800D558C (volatile unsigned char *): SPU MMIO base (0x1F801C00),
 *     +0x1AA (u16) SPUCNT control register (bits 8..13 updated, mask 0xC0FF)
 */

#define H(p, o) (*(volatile unsigned short *)((p) + (o)))

extern volatile unsigned char *D_800D558C;

signed int func_800C2DD8(signed int clock)
{
    signed int clamped;
    unsigned short reg;

    if (clock < 0) {
        clamped = 0;
    } else if (clock < 0x40) {
        clamped = clock;
    } else {
        clamped = 0x3F;
    }

    reg = H(D_800D558C, 0x1AA);
    reg = (unsigned short)((reg & 0xC0FFU) | (((unsigned int)clamped & 0x3FU) << 8));
    H(D_800D558C, 0x1AA) = reg;

    return clamped;
}
