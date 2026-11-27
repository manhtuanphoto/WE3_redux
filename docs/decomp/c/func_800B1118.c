/*
 * func_800B1118
 *
 * Purpose:
 *   PS1 BIOS A0 vector trampoline: A(2Ah) - memcpy(dst, src, size).
 *   Loads $t2 = 0xA0, $t1 = 0x2A and tail-jumps to 0x000000A0, forwarding
 *   $a0..$a2 to copy `size` bytes from `src` to `dst`.
 *
 * Arguments:
 *   dst  (void *): Destination buffer ($a0)
 *   src  (const void *): Source buffer ($a1)
 *   size (int): Number of bytes to copy ($a2)
 *
 * Return value:
 *   (void *) `dst` pointer returned in $v0 by BIOS A(2Ah).
 */

void *func_800B1118(void *dst, const void *src, int size)
{
    /* BIOS call: $t2 = 0xA0, $t1 = 0x2A (memcpy), jr $t2 */
    /* UNCLEAR: standard C cannot set $t1 = 0x2A and tail-jump to 0x000000A0 without inline asm. */
    (void)src;
    (void)size;
    return dst;
}
