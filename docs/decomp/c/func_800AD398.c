/*
 * func_800AD398
 *
 * Purpose:
 *   PS1 BIOS B0 vector trampoline: B(12h) - InitPAD(bufA, lenA, bufB, lenB).
 *   Loads $t2 = 0xB0, $t1 = 0x12 and tail-jumps to 0x000000B0, forwarding
 *   $a0..$a3 and $ra.
 *
 * Arguments:
 *   bufA (void *): Controller port 1 receive buffer ($a0)
 *   lenA (int): Buffer A size ($a1)
 *   bufB (void *): Controller port 2 receive buffer ($a2)
 *   lenB (int): Buffer B size ($a3)
 *
 * Return value:
 *   int: BIOS status returned in $v0
 */

int func_800AD398(void *bufA, int lenA, void *bufB, int lenB)
{
    /* BIOS call: $t2 = 0xB0, $t1 = 0x12 (InitPAD), jr $t2 */
    /* UNCLEAR: standard C cannot set $t1 = 0x12 and tail-jump to 0x000000B0 without inline asm. */
    (void)bufA;
    (void)lenA;
    (void)bufB;
    (void)lenB;
    return 0;
}
