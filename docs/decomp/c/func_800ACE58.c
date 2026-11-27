/*
 * func_800ACE58
 *
 * Purpose:
 *   PS1 BIOS B0:0x0C (EnableEvent) trampoline stub.
 *   Sets $t1 = 0x0C and jumps directly to the BIOS B0 vector (0x000000B0),
 *   forwarding $a0 (event handle) and returning to the caller via $ra.
 *
 * Arguments:
 *   event (unsigned int) - BIOS event descriptor returned by OpenEvent
 *                          (e.g. 0xF0000009 SPU event in func_800C12C0)
 *
 * Return value:
 *   None (void in callers; underlying BIOS B0:0x0C returns status in $v0)
 */

void func_800ACE58(unsigned int event)
{
    register unsigned int a0 __asm__("$4") = event;
    register unsigned int t1 __asm__("$9") = 0x0CU;
    register unsigned int t2 __asm__("$10") = 0x000000B0U;

    /* Tail-jump to PS1 BIOS B0 vector with function ID $t1 = 0x0C (EnableEvent) */
    __asm__ volatile("jr %0" : : "r"(t2), "r"(t1), "r"(a0) : "memory");
}
