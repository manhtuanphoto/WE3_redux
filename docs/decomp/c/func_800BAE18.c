/*
 * func_800BAE18
 *
 * Purpose:
 *   PS1 BIOS B0 vector trampoline: B(17h) - ReturnFromException().
 *   Loads $t2 = 0xB0, $t1 = 0x17 and tail-jumps to 0x000000B0 to restore CPU
 *   registers from the current Thread Control Block (TCB) and return from an
 *   exception/interrupt handler.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (does not return to caller in normal exception flow)
 */

void func_800BAE18(void)
{
    /* BIOS call: $t2 = 0xB0, $t1 = 0x17 (ReturnFromException), jr $t2 */
    /* UNCLEAR: standard C cannot set $t1 = 0x17 and tail-jump to 0x000000B0 without inline asm. */
}
