/*
 * func_800ACE38
 *
 * Purpose:
 *   PS1 BIOS B0 vector trampoline: B(08h) - OpenEvent(class_id, spec, mode, func).
 *   Loads $t2 = 0xB0, $t1 = 0x08 and tail-jumps to 0x000000B0, forwarding
 *   $a0..$a3 to register an event descriptor in the BIOS event table.
 *
 * Arguments:
 *   class_id (unsigned int): Event class descriptor ($a0, e.g. 0xF0000009)
 *   spec     (int): Event spec mask ($a1)
 *   mode     (int): Event mode ($a2, e.g. 0x1000 = callback, 0x2000 = no intr)
 *   func     (void (*)(void)): Optional callback function pointer ($a3)
 *
 * Return value:
 *   (unsigned int) Event descriptor handle (0xF1000000+) or 0xFFFFFFFF on failure ($v0).
 */

unsigned int func_800ACE38(unsigned int class_id, int spec, int mode, void (*func)(void))
{
    /* BIOS call: $t2 = 0xB0, $t1 = 0x08 (OpenEvent), jr $t2 */
    /* UNCLEAR: standard C cannot set $t1 = 0x08 and tail-jump to 0x000000B0 without inline asm. */
    (void)class_id;
    (void)spec;
    (void)mode;
    (void)func;
    return 0;
}
