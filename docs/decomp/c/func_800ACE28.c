/*
 * func_800ACE28
 *
 * Purpose:
 *   PS1 BIOS A0 vector trampoline: A(70h) - _bu_init().
 *   Loads $t2 = 0xA0, $t1 = 0x70 and tail-jumps to 0x000000A0 to initialize
 *   the Memory Card BIOS subsystem.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 */

void func_800ACE28(void)
{
    /* BIOS call: $t2 = 0xA0, $t1 = 0x70 (_bu_init), jr $t2 */
    /* UNCLEAR: standard C cannot set $t1 = 0x70 and tail-jump to 0x000000A0 without inline asm. */
}
