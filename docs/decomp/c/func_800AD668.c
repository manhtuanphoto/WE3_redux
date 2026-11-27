/*
 * func_800AD668
 *
 * Purpose:
 *   PS1 BIOS A0 vector trampoline: A(2Bh) - memset(dst, c, len).
 *   Loads $t2 = 0xA0, $t1 = 0x2B and tail-jumps to 0x000000A0, forwarding
 *   $a0, $a1, $a2, and $ra.
 *
 * Arguments:
 *   dst (void *): Destination buffer ($a0)
 *   c (int): Fill byte ($a1)
 *   len (int): Byte count ($a2)
 *
 * Return value:
 *   void *: Original dst pointer (returned by BIOS in $v0)
 */

void *func_800AD668(void *dst, int c, int len)
{
    /* BIOS call: $t2 = 0xA0, $t1 = 0x2B (memset), jr $t2 */
    /* UNCLEAR: standard C cannot set $t1 = 0x2B and tail-jump to 0x000000A0 without inline asm. */
    unsigned char *d = (unsigned char *)dst;
    int i;

    for (i = 0; i < len; i++) {
        d[i] = (unsigned char)c;
    }
    return dst;
}
