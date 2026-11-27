/*
 * func_800AD01C
 *
 * Purpose:
 *   Clears a bitmask looked up from D_800CEA4C[index & 0xFFFF] in the 32-bit
 *   register/field at offset +0x4 of the pointer D_800CEA44 (likely PS1 I_MASK
 *   at 0x1F801074 when D_800CEA44 points to 0x1F801070).
 *
 * Arguments:
 *   index (unsigned short / int): Mask table index (low 16 bits used)
 *
 * Return value:
 *   int: Always returns 1
 *
 * Globals read:
 *   D_800CEA44 (volatile unsigned int *): Base pointer (+0x4 read/written)
 *   D_800CEA4C (const unsigned int[]): Bitmask lookup table
 */

extern volatile unsigned int *D_800CEA44;
extern const unsigned int D_800CEA4C[];

int func_800AD01C(int index)
{
    unsigned int idx = (unsigned int)index & 0xFFFFU;
    unsigned int mask = D_800CEA4C[idx];

    /* UNCLEAR: exact identity of D_800CEA44 (likely 0x1F801070 I_STAT/I_MASK block) */
    D_800CEA44[1] &= ~mask;
    return 1;
}
