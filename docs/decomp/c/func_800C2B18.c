/*
 * func_800C2B18 - Read/modify 24-bit PS1 SPU voice register shadow pair
 *
 * Purpose:
 *   Operates on a 24-bit bitmask split across two 16-bit halfword entries
 *   (low 16 bits at table[reg_lo], high 8 bits at table[reg_hi] & 0xFF) in either
 *   D_800F0D20 (when D_800D5578 & 1) or D_800D558C. Supports clearing bits (mode 0),
 *   setting bits (mode 1), and overwriting (mode 8). When D_800D5578 & 1 is set,
 *   also marks the corresponding dirty bit in D_800D5144.
 *
 * Arguments:
 *   op     (int)          - operation: 0 = clear bits, 1 = set bits, 8 = write value
 *   mask   (unsigned int) - 24-bit value/mask (bits 0..15 -> reg_lo, bits 16..23 -> reg_hi)
 *   reg_lo (int)          - halfword index for low 16 bits
 *   reg_hi (int)          - halfword index for high 8 bits
 *
 * Return value:
 *   unsigned int - resulting 24-bit register value (& 0xFFFFFF), or 0xFFFFFF on invalid op.
 *
 * Globals read & written:
 *   D_800D5578 (u32)  - bit 0 selects shadow table D_800F0D20 (1) vs *D_800D558C (0)
 *   D_800F0D20 (u16[]) - primary SPU register shadow table
 *   D_800D558C (u16*) - alternate SPU register shadow table pointer
 *   D_800D5144 (u32)  - dirty register bitmask: sets bit ((reg_lo - 0xC6) >> 1)
 */

extern unsigned int D_800D5578;
extern unsigned short D_800F0D20[];
extern unsigned short *D_800D558C;
extern unsigned int D_800D5144;

unsigned int func_800C2B18(int op, unsigned int mask, int reg_lo, int reg_hi)
{
    unsigned short *table;
    unsigned int cur;

    table = (D_800D5578 & 1) ? D_800F0D20 : D_800D558C;
    cur = table[reg_lo] | ((table[reg_hi] & 0xFF) << 16);

    if (op == 1) {
        if (D_800D5578 & 1) {
            D_800F0D20[reg_lo] |= (unsigned short)mask;
            D_800F0D20[reg_hi] |= (unsigned short)((mask >> 16) & 0xFF);
            D_800D5144 |= 1U << ((reg_lo - 0xC6) >> 1);
        } else {
            D_800D558C[reg_lo] |= (unsigned short)mask;
            D_800D558C[reg_hi] |= (unsigned short)((mask >> 16) & 0xFF);
        }
        cur |= (mask & 0xFFFFFF);
    } else if (op == 0) {
        if (D_800D5578 & 1) {
            D_800F0D20[reg_lo] &= (unsigned short)(~mask);
            D_800F0D20[reg_hi] &= (unsigned short)(~((mask >> 16) & 0xFF));
            D_800D5144 |= 1U << ((reg_lo - 0xC6) >> 1);
        } else {
            D_800D558C[reg_lo] &= (unsigned short)(~mask);
            D_800D558C[reg_hi] &= (unsigned short)(~((mask >> 16) & 0xFF));
        }
        cur &= ~(mask & 0xFFFFFF);
    } else if (op == 8) {
        if (D_800D5578 & 1) {
            D_800F0D20[reg_lo] = (unsigned short)mask;
            D_800F0D20[reg_hi] = (unsigned short)((mask >> 16) & 0xFF);
            D_800D5144 |= 1U << ((reg_lo - 0xC6) >> 1);
        } else {
            D_800D558C[reg_lo] = (unsigned short)mask;
            D_800D558C[reg_hi] = (unsigned short)((mask >> 16) & 0xFF);
        }
        cur = mask & 0xFFFFFF;
    } else {
        return 0xFFFFFF;
    }

    return cur & 0xFFFFFF;
}
