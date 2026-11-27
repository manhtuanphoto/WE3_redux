/*
 * func_800C2AE8
 *
 * Purpose:
 *   Convenience wrapper around func_800C2B18 for the 24-bit PS1 SPU shadow
 *   register pair at halfword indices 0xCA (low 16 bits) and 0xCB (high 8 bits).
 *   Sets, clears, or writes the 24-bit register mask depending on `op` and
 *   marks dirty bit 2 ((0xCA - 0xC6) >> 1) in D_800D5144 when shadow mode is active.
 *
 * Arguments:
 *   op   (int)          - operation passed to func_800C2B18 (0=clear, 1=set, 8=write)
 *   mask (unsigned int) - 24-bit voice mask/value
 *
 * Return value:
 *   unsigned int: resulting 24-bit register value (& 0xFFFFFF), or 0xFFFFFF on invalid op
 */

extern unsigned int func_800C2B18(int op, unsigned int mask, int reg_lo, int reg_hi);

unsigned int func_800C2AE8(int op, unsigned int mask)
{
    /* UNCLEAR: exact SPU 24-bit register corresponding to shadow indices 0xCA / 0xCB */
    return func_800C2B18(op, mask, 0xCA, 0xCB);
}
