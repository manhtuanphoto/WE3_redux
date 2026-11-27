/*
 * func_800BAE38
 *
 * Purpose:
 *   PS1 BIOS B0 vector trampoline: B(19h) - SetCustomExitFromException(addr).
 *   Loads $t2 = 0xB0, $t1 = 0x19 and tail-jumps to 0x000000B0, forwarding
 *   $a0 (pointer to jmp_buf / exit context structure) to install a custom
 *   exception exit target.
 *
 * Arguments:
 *   addr (void *): Pointer to saved context / exit structure ($a0)
 *
 * Return value:
 *   None (void)
 */

void func_800BAE38(void *addr)
{
    /* BIOS call: $t2 = 0xB0, $t1 = 0x19 (SetCustomExitFromException), jr $t2 */
    /* UNCLEAR: standard C cannot set $t1 = 0x19 and tail-jump to 0x000000B0 without inline asm. */
    (void)addr;
}
