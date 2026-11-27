/*
 * func_800B6E78
 *
 * Purpose:
 *   PS1 BIOS B0:0x07 (DeliverEvent) trampoline stub.
 *   Sets $t1 = 0x07 and jumps directly to the BIOS B0 vector (0x000000B0),
 *   triggering any event handlers matching the given event class and spec.
 *
 * Arguments:
 *   event_id (unsigned int) - BIOS event class (e.g. 0xF0000003 HwCdRom,
 *                             0xF0000009 HwSPU)
 *   flags    (unsigned int) - BIOS event spec bitmask (e.g. 0x20 EvSpCOMP,
 *                             0x40 EvSpERROR/EvSpACK)
 *
 * Return value:
 *   None (void in callers; underlying BIOS B0:0x07 returns status in $v0)
 */

void func_800B6E78(unsigned int event_id, unsigned int flags)
{
    register unsigned int a0 __asm__("$4") = event_id;
    register unsigned int a1 __asm__("$5") = flags;
    register unsigned int t1 __asm__("$9") = 0x07U;
    register unsigned int t2 __asm__("$10") = 0x000000B0U;

    /* Tail-jump to PS1 BIOS B0 vector with function ID $t1 = 0x07 (DeliverEvent) */
    __asm__ volatile("jr %0" : : "r"(t2), "r"(t1), "r"(a0), "r"(a1) : "memory");
}
