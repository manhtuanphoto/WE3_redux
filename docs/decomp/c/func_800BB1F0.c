/*
 * func_800BB1F0
 *
 * Purpose:
 *   Zero-fills `word_count` consecutive 32-bit words starting at `dst`.
 *   Does nothing if `word_count == 0`. (Identical implementation to
 *   func_800BAF48, likely from a separate translation unit.)
 *
 * Arguments:
 *   dst (unsigned int *): destination 32-bit-aligned buffer
 *   word_count (unsigned int): number of 32-bit words to zero
 *
 * Return value:
 *   None (void)
 */

void func_800BB1F0(unsigned int *dst, unsigned int word_count)
{
    while (word_count != 0U) {
        *dst++ = 0U;
        word_count--;
    }
}
