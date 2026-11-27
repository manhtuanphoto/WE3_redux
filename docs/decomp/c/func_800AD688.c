/*
 * func_800AD688
 *
 * Purpose:
 *   PS1 BIOS A0 vector trampoline: A(30h) - srand(seed).
 *   Loads $t2 = 0xA0, $t1 = 0x30 and tail-jumps to 0x000000A0, forwarding
 *   $a0 (seed) to seed the PS1 BIOS pseudo-random number generator.
 *
 * Arguments:
 *   seed (unsigned int): PRNG seed value ($a0)
 *
 * Return value:
 *   None (void)
 */

void func_800AD688(unsigned int seed)
{
    /* BIOS call: $t2 = 0xA0, $t1 = 0x30 (srand), jr $t2 */
    /* UNCLEAR: standard C cannot set $t1 = 0x30 and tail-jump to 0x000000A0 without inline asm. */
    (void)seed;
}
