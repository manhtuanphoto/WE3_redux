/*
 * func_800B1128
 *
 * Purpose:
 *   Fixed-point integer square root using PS1 GTE leading zero/one count
 *   coprocessor registers (cop2 $30 LZCS / $31 LZCR) and lookup table
 *   D_800CEBE4 (PsyQ SquareRoot0 pattern).
 *
 * Arguments:
 *   val (signed int) - input integer value (returns 0 if val == 0 / LZCR == 32)
 *
 * Return value:
 *   Unsigned/non-negative square root result ((u32)(D_800CEBE4[mant - 0x40] << shift) >> 12).
 *
 * Hardware / GTE registers used:
 *   cop2 $30 (LZCS) - written with val
 *   cop2 $31 (LZCR) - read to obtain leading bit count (0..32)
 *
 * Global fields read:
 *   D_800CEBE4 (s16[]) - square-root mantissa lookup table (indexed by mantissa - 0x40)
 */

extern signed short D_800CEBE4[];

unsigned int func_800B1128(signed int val)
{
    int lzcr;
    int lz_even;
    int shift;
    int norm_shift;
    int mantissa;
    signed int lut_val;

    /* GTE: mtc2 val, $30 (LZCS); mfc2 lzcr, $31 (LZCR) */
    if (val == 0) {
        return 0U;
    }

    /* Leading zero/one count matching GTE LZCS/LZCR hardware behavior */
    {
        unsigned int u = (val < 0) ? ~(unsigned int)val : (unsigned int)val;
        lzcr = 0;
        while ((u & 0x80000000U) == 0U && lzcr < 32) {
            u <<= 1;
            lzcr++;
        }
    }

    if (lzcr == 32) {
        return 0U;
    }

    lz_even = lzcr & ~1;
    shift = (31 - lz_even) >> 1;
    norm_shift = lz_even - 24;

    if (norm_shift >= 0) {
        mantissa = (int)((unsigned int)val << norm_shift);
    } else {
        mantissa = val >> (24 - lz_even);
    }

    lut_val = (signed int)D_800CEBE4[mantissa - 0x40];
    return (unsigned int)(lut_val << shift) >> 12;
}
