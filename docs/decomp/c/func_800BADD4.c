/*
 * func_800BADD4
 *
 * Purpose:
 *   Zero-fills `wordCount` consecutive 32-bit words starting at `dst`.
 *   If `wordCount == 0`, returns immediately without writing.
 *
 *   Note: The assembly has `blez $t0, D_800D7B3C` in the delay slot of `jr $ra`
 *   followed by `tltu $v0, $at, 366`, which appears to be non-executed data
 *   or misdisassembled trailing padding after the return instruction.
 *
 * Arguments:
 *   dst (unsigned int *): Destination word-aligned buffer
 *   wordCount (int): Number of 32-bit words to clear
 *
 * Return value:
 *   None (void)
 */

void func_800BADD4(unsigned int *dst, int wordCount)
{
    int remaining;

    if (wordCount == 0) {
        return;
    }

    remaining = wordCount - 1;
    do {
        *dst++ = 0;
        remaining--;
    } while (remaining != -1);

    /* UNCLEAR: trailing instructions after/in delay slot of jr $ra:
     * blez $t0, D_800D7B3C; tltu $v0, $at, 366 */
}
