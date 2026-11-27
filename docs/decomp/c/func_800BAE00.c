/*
 * func_800BAE00
 *
 * Purpose:
 *   PS1 BIOS A0 vector trampoline: A(72h) - CdRemove() / _96_remove().
 *   Loads $t2 = 0xA0, $t1 = 0x72 and tail-jumps to 0x000000A0 to remove/reset
 *   the BIOS CD-ROM subsystem handlers.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 */

void func_800BAE00(void)
{
    /* BIOS call: $t2 = 0xA0, $t1 = 0x72 (CdRemove / _96_remove), jr $t2 */
    /* UNCLEAR: standard C cannot set $t1 = 0x72 and tail-jump to 0x000000A0 without inline asm. */
}
