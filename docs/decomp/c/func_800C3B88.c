/*
 * func_800C3B88
 *
 * Purpose:
 *   PS1 BIOS B0:0x0A (WaitEvent) trampoline stub.
 *   Sets $t1 = 0x0A and jumps directly to the BIOS B0 vector (0x000000B0),
 *   blocking until the specified BIOS event occurs.
 *
 * Arguments:
 *   event (int) - BIOS event descriptor returned by OpenEvent
 *                 (e.g. D_800D5114 SPU transfer event in func_800C39E8)
 *
 * Return value:
 *   None (void in callers; underlying BIOS B0:0x0A returns status in $v0)
 */

void func_800C3B88(int event)
{
    register int a0 __asm__("$4") = event;
    register unsigned int t1 __asm__("$9") = 0x0AU;
    register unsigned int t2 __asm__("$10") = 0x000000B0U;

    /* Tail-jump to PS1 BIOS B0 vector with function ID $t1 = 0x0A (WaitEvent) */
    __asm__ volatile("jr %0" : : "r"(t2), "r"(t1), "r"(a0) : "memory");
}
