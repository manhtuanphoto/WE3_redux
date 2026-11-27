/*
 * func_800BAE28
 *
 * Purpose:
 *   PS1 BIOS B0 vector trampoline: B(18h) - SetDefaultExitFromException().
 *   Loads $t2 = 0xB0, $t1 = 0x18 and tail-jumps to 0x000000B0 to reset the
 *   BIOS exception exit behavior to the default handler.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 */

void func_800BAE28(void)
{
    /* BIOS call: $t2 = 0xB0, $t1 = 0x18 (SetDefaultExitFromException), jr $t2 */
    /* UNCLEAR: standard C cannot set $t1 = 0x18 and tail-jump to 0x000000B0 without inline asm. */
}
