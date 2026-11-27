/*
 * func_800AD678
 *
 * Purpose:
 *   PS1 BIOS A0 vector trampoline: A(2Fh) - rand().
 *   Loads $t2 = 0xA0, $t1 = 0x2F and tail-jumps to 0x000000A0, returning
 *   a pseudo-random integer in $v0.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   (int) Pseudo-random integer from the PS1 BIOS PRNG.
 */

int func_800AD678(void)
{
    /* BIOS call: $t2 = 0xA0, $t1 = 0x2F (rand), jr $t2 */
    /* UNCLEAR: standard C cannot set $t1 = 0x2F and tail-jump to 0x000000A0 without inline asm. */
    return 0;
}
