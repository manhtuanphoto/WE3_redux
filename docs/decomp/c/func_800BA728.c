/*
 * func_800BA728
 *
 * Purpose:
 *   PS1 BIOS C0:0x0A (ChangeClearRCnt) trampoline stub.
 *   Sets $t1 = 0x0A and jumps directly to the BIOS C0 vector (0x000000C0),
 *   configuring whether the BIOS kernel automatically acknowledges the given
 *   root counter / VBlank IRQ or leaves it to user handlers.
 *
 * Arguments:
 *   channel (int) - root counter index (0..2 = RCnt0..2, 3 = VBlank)
 *   disable (int) - 0 = pass IRQ to user handler, 1 = BIOS auto-acknowledges
 *
 * Return value:
 *   None (void in callers; underlying BIOS C0:0x0A returns previous flag in $v0)
 */

void func_800BA728(int channel, int disable)
{
    register int a0 __asm__("$4") = channel;
    register int a1 __asm__("$5") = disable;
    register unsigned int t1 __asm__("$9") = 0x0AU;
    register unsigned int t2 __asm__("$10") = 0x000000C0U;

    /* Tail-jump to PS1 BIOS C0 vector with function ID $t1 = 0x0A (ChangeClearRCnt) */
    __asm__ volatile("jr %0" : : "r"(t2), "r"(t1), "r"(a0), "r"(a1) : "memory");
}
