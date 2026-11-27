/*
 * func_800AD3A8
 *
 * Purpose:
 *   PS1 BIOS B0 vector trampoline: B(13h) - StartPAD().
 *   Loads $t2 = 0xB0, $t1 = 0x13 and tail-jumps to 0x000000B0.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   int: BIOS status returned in $v0
 */

int func_800AD3A8(void)
{
    /* BIOS call: $t2 = 0xB0, $t1 = 0x13 (StartPAD), jr $t2 */
    /* UNCLEAR: standard C cannot set $t1 = 0x13 and tail-jump to 0x000000B0 without inline asm. */
    return 0;
}
