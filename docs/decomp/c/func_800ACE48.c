/*
 * func_800ACE48
 *
 * Purpose:
 *   PS1 BIOS B0 vector trampoline: B(0Bh) - TestEvent(event).
 *   Loads $t2 = 0xB0, $t1 = 0x0B and tail-jumps to 0x000000B0, forwarding
 *   $a0 to test whether the given BIOS event has occurred (non-blocking).
 *
 * Arguments:
 *   event (unsigned int): BIOS event descriptor handle ($a0)
 *
 * Return value:
 *   (int) 1 if the event has occurred, 0 otherwise ($v0).
 */

int func_800ACE48(unsigned int event)
{
    /* BIOS call: $t2 = 0xB0, $t1 = 0x0B (TestEvent), jr $t2 */
    /* UNCLEAR: standard C cannot set $t1 = 0x0B and tail-jump to 0x000000B0 without inline asm. */
    (void)event;
    return 0;
}
