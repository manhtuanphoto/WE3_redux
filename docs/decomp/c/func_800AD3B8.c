/*
 * func_800AD3B8
 *
 * Purpose:
 *   PS1 BIOS B0 vector trampoline: B(14h) - StopPAD().
 *   Loads $t2 = 0xB0, $t1 = 0x14 and tail-jumps to 0x000000B0.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 */

void func_800AD3B8(void)
{
    /* BIOS call: $t2 = 0xB0, $t1 = 0x14 (StopPAD), jr $t2 */
    /* UNCLEAR: standard C cannot set $t1 = 0x14 and tail-jump to 0x000000B0 without inline asm. */
}
