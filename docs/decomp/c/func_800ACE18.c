/*
 * func_800ACE18
 *
 * Purpose:
 *   PS1 BIOS A0 vector trampoline: A(39h) - InitHeap(addr, size).
 *   Loads $t2 = 0xA0, $t1 = 0x39 and tail-jumps to 0x000000A0, forwarding
 *   caller arguments ($a0, $a1) and return address ($ra).
 *
 * Arguments:
 *   addr (void *): Heap base address ($a0)
 *   size (unsigned int): Heap size in bytes ($a1)
 *
 * Return value:
 *   None (void)
 */

void func_800ACE18(void *addr, unsigned int size)
{
    /* BIOS call: $t2 = 0xA0, $t1 = 0x39 (InitHeap), jr $t2 */
    /* UNCLEAR: standard C cannot set $t1 = 0x39 and tail-jump to 0x000000A0 without inline asm. */
    (void)addr;
    (void)size;
}
