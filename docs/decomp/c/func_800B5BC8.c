/*
 * func_800B5BC8
 *
 * Purpose:
 *   Data block (`dlabel`) containing two non-instruction 32-bit words
 *   immediately preceding func_800B5BD0 (InitGeom).
 *   UNCLEAR: Exact meaning of the two words (0x19097350, 0x004162B3);
 *   appears to be PsyQ library/build signature or padding data, similar to
 *   trailing non-instruction words in func_800B8BA8.s.
 *
 * Arguments / Return value:
 *   N/A (not executable code)
 */

const unsigned int func_800B5BC8[2] = {
    0x19097350U, /* UNCLEAR: non-instruction word / invalid MIPS instruction */
    0x004162B3U
};
