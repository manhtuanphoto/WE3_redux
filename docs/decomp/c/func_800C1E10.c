/*
 * func_800C1E10
 *
 * Purpose:
 *   Busy-wait / hardware timing delay loop (PsyQ SPU wait helper). Initializes
 *   a local accumulator to 13 (0xD) and multiplies it by 13 for 60 (0x3C)
 *   iterations, discarding the result.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 */

void func_800C1E10(void)
{
    volatile signed int acc = 0xD;
    volatile signed int i;

    for (i = 0; i < 0x3C; i++) {
        acc *= 0xD;
    }
}
