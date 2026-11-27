/*
 * func_800AD658
 *
 * Purpose:
 *   PS1 BIOS A0 vector trampoline: A(28h) - bcopy(src, dst, len).
 *   Loads $t2 = 0xA0, $t1 = 0x28 and tail-jumps to 0x000000A0, forwarding
 *   $a0, $a1, $a2, and $ra.
 *
 * Arguments:
 *   src (const void *): Source buffer ($a0)
 *   dst (void *): Destination buffer ($a1)
 *   len (int): Byte count ($a2)
 *
 * Return value:
 *   None (void)
 */

void func_800AD658(const void *src, void *dst, int len)
{
    /* BIOS call: $t2 = 0xA0, $t1 = 0x28 (bcopy), jr $t2 */
    /* UNCLEAR: standard C cannot set $t1 = 0x28 and tail-jump to 0x000000A0 without inline asm. */
    const unsigned char *s = (const unsigned char *)src;
    unsigned char *d = (unsigned char *)dst;
    int i;

    for (i = 0; i < len; i++) {
        d[i] = s[i];
    }
}
