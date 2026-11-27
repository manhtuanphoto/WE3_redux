/*
 * func_800AD698
 *
 * Purpose:
 *   PS1 BIOS A0 vector trampoline: A(3Fh) - printf(fmt, ...).
 *   Loads $t2 = 0xA0, $t1 = 0x3F and tail-jumps to 0x000000A0, forwarding
 *   caller arguments ($a0..$a3 and stack variadic args) to the PS1 BIOS
 *   TTY debug printf routine.
 *
 * Arguments:
 *   fmt (const char *): Format string ($a0)
 *   ...               : Variadic format arguments ($a1..$a3, stack)
 *
 * Return value:
 *   (int) Value returned in $v0 by BIOS A(3Fh).
 */

int func_800AD698(const char *fmt, ...)
{
    /* BIOS call: $t2 = 0xA0, $t1 = 0x3F (printf), jr $t2 */
    /* UNCLEAR: standard C cannot set $t1 = 0x3F and tail-jump to 0x000000A0 with forwarded variadic args without inline asm. */
    (void)fmt;
    return 0;
}
