/*
 * func_800AD3E8
 *
 * Purpose:
 *   PS1 BIOS C0:0x02 (SysEnqIntRP) trampoline stub.
 *   Sets $t1 = 0x02 and jumps directly to the BIOS C0 vector (0x000000C0),
 *   enqueuing an interrupt handler chain node at the specified priority.
 *
 * Arguments:
 *   priority (int)            - interrupt priority queue index (e.g. 1 in func_800AD23C)
 *   node     (unsigned int *) - pointer to IntRP struct (next, func1, func2, ...)
 *
 * Return value:
 *   None (void in callers; underlying BIOS C0:0x02 returns status in $v0)
 */

void func_800AD3E8(int priority, unsigned int *node)
{
    register int a0 __asm__("$4") = priority;
    register unsigned int *a1 __asm__("$5") = node;
    register unsigned int t1 __asm__("$9") = 0x02U;
    register unsigned int t2 __asm__("$10") = 0x000000C0U;

    /* Tail-jump to PS1 BIOS C0 vector with function ID $t1 = 0x02 (SysEnqIntRP) */
    __asm__ volatile("jr %0" : : "r"(t2), "r"(t1), "r"(a0), "r"(a1) : "memory");
}
